/*
** Copyright 2020 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#include <amqpprox_logging.h>

#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace Bloomberg {
namespace amqpprox {

std::atomic<int> Logging::s_consoleVerbosity{0};
std::atomic<int> Logging::s_fileVerbosity{3};

boost::shared_ptr<Logging::AsyncConsoleSink> Logging::s_consoleSink;
boost::shared_ptr<Logging::AsyncFileSink>    Logging::s_fileSink;

namespace {

struct severity_tag;

// Custom formating for the verbosity/severity information
boost::log::formatting_ostream &
operator<<(boost::log::formatting_ostream                    &strm,
           const boost::log::to_log_manip<int, severity_tag> &manip)
{
    static const char *strings[] = {
        "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

    int level = manip.get();
    if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << level;

    return strm;
}

}

void Logging::start(const std::string &logDirectory)
{
    using namespace boost::log;

    auto core = boost::log::core::get();
    boost::log::add_common_attributes();

    core->add_global_attribute("UTCTimeStamp", attributes::utc_clock());
    core->add_global_attribute("Scope", attributes::named_scope());

    s_consoleSink = boost::make_shared<AsyncConsoleSink>();
    s_consoleSink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
    s_consoleSink->locked_backend()->auto_flush(true);

    int consoleVerb = s_consoleVerbosity;
    s_consoleSink->set_filter(expressions::attr<int>("Severity") <=
                              consoleVerb);
    s_consoleSink->set_formatter(
        expressions::stream
        << expressions::attr<int, severity_tag>("Severity")
        << expressions::if_(expressions::has_attr<std::string>(
               "Vhost"))[expressions::stream
                         << " [" << expressions::attr<std::string>("Vhost")
                         << "]"]
        << expressions::if_(expressions::has_attr<uint64_t>(
               "ConnID"))[expressions::stream
                          << " " << expressions::attr<uint64_t>("ConnID")]
        << ": " << expressions::smessage);

    auto backend = boost::make_shared<sinks::text_file_backend>(
        keywords::file_name     = logDirectory + "/amqpprox_%Y%m%d_%5N.log",
        keywords::rotation_size = 32 * 1024 * 1024,
        keywords::time_based_rotation =
            sinks::file::rotation_at_time_point(0, 0, 0));
    s_fileSink = boost::make_shared<AsyncFileSink>(backend);

    int fileVerb = s_fileVerbosity;
    s_fileSink->set_filter(expressions::attr<int>("Severity") <= fileVerb);
    s_fileSink->set_formatter(
        expressions::stream
        << expressions::format_date_time<boost::posix_time::ptime>(
               "UTCTimeStamp", "%Y-%m-%dT%H:%M:%S.%fZ")
        << " " << expressions::attr<unsigned int>("LineID") << " "
        << expressions::attr<process_id>("ProcessID") << "-"
        << expressions::attr<thread_id>("ThreadID")
        << expressions::if_(expressions::has_attr<std::string>(
               "Scope"))[expressions::stream
                         << " " << expressions::attr<std::string>("Scope")]
        << " " << expressions::attr<int, severity_tag>("Severity")
        << expressions::if_(expressions::has_attr<std::string>(
               "Vhost"))[expressions::stream
                         << " [" << expressions::attr<std::string>("Vhost")
                         << "]"]
        << expressions::if_(expressions::has_attr<uint64_t>(
               "ConnID"))[expressions::stream
                          << " " << expressions::attr<uint64_t>("ConnID")]
        << ": " << expressions::smessage);

    s_fileSink->locked_backend()->auto_flush(true);
    s_fileSink->locked_backend()->set_file_collector(
        sinks::file::make_collector(keywords::target   = logDirectory,
                                    keywords::max_size = 1024 * 1024 * 1024,
                                    keywords::min_free_space =
                                        100 * 1024 * 1024));
    s_fileSink->locked_backend()->scan_for_files();

    core->add_sink(s_consoleSink);
    core->add_sink(s_fileSink);
}

void Logging::stop()
{
    auto core = boost::log::core::get();

    core->remove_sink(s_consoleSink);
    core->remove_sink(s_fileSink);

    s_consoleSink->stop();
    s_fileSink->stop();

    s_consoleSink->flush();
    s_fileSink->flush();

    s_consoleSink.reset();
    s_fileSink.reset();
}

void Logging::setConsoleVerbosity(int verbosity)
{
    using namespace boost::log;
    s_consoleVerbosity = verbosity;
    s_consoleSink->set_filter(expressions::attr<int>("Severity") <= verbosity);
}

void Logging::setFileVerbosity(int verbosity)
{
    using namespace boost::log;
    s_fileVerbosity = verbosity;
    s_fileSink->set_filter(expressions::attr<int>("Severity") <= verbosity);
}

}
}
