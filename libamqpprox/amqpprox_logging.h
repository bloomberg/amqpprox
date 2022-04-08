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
#ifndef BLOOMBERG_AMQPPROX_LOGGING
#define BLOOMBERG_AMQPPROX_LOGGING

#include <boost/log/common.hpp>

#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/sinks/drop_on_overflow.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>

#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_feature.hpp>

#include <boost/shared_ptr.hpp>

#include <atomic>

#define LOG_TRACE BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_TRACE)
#define LOG_DEBUG BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_DEBUG)
#define LOG_INFO BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_INFO)
#define LOG_WARN BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_WARN)
#define LOG_ERROR BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_ERROR)
#define LOG_FATAL BOOST_LOG_SEV(Logging::get(), Logging::AMQPPROX_LOG_FATAL)
#define LOG_SEV(sev) BOOST_LOG_SEV(Logging::get(), sev)

namespace Bloomberg {
namespace amqpprox {

/**
 * \brief Manages logging across application
 */
class Logging {
  public:
    using AsyncConsoleSink = boost::log::sinks::asynchronous_sink<
        boost::log::sinks::text_ostream_backend,
        boost::log::sinks::
            bounded_fifo_queue<4096, boost::log::sinks::drop_on_overflow>>;

    using AsyncFileSink = boost::log::sinks::asynchronous_sink<
        boost::log::sinks::text_file_backend,
        boost::log::sinks::
            bounded_fifo_queue<4096, boost::log::sinks::drop_on_overflow>>;

  private:
    static std::atomic<int>                    s_consoleVerbosity;
    static std::atomic<int>                    s_fileVerbosity;
    static boost::shared_ptr<AsyncConsoleSink> s_consoleSink;
    static boost::shared_ptr<AsyncFileSink>    s_fileSink;

  public:
    enum AMQPPROX_LOG_LEVELS {
        AMQPPROX_LOG_TRACE = 5,
        AMQPPROX_LOG_DEBUG = 4,
        AMQPPROX_LOG_INFO  = 3,
        AMQPPROX_LOG_WARN  = 2,
        AMQPPROX_LOG_ERROR = 1,
        AMQPPROX_LOG_FATAL = 0
    };

    /**
     * \return logger which supports severity level
     */
    static inline boost::log::sources::severity_logger_mt<> &get()
    {
        static boost::log::sources::severity_logger_mt<> logger;
        return logger;
    }

    /**
     * \brief Initialize logging component and start logging
     * \param logDirectory specifies location of log directory,
     * where all amqpprox logs will be stored. All the logs will be logged in
     * UTC time.
     */
    static void start(const std::string &logDirectory);
    /**
     * \brief Deinitialize logging component and stop logging
     */
    static void stop();
    /**
     * \brief Set console verbosity
     * \param verbosity can be any integer value from enum
     * `AMQPPROX_LOG_LEVELS`
     */
    static void setConsoleVerbosity(int verbosity);
    /**
     * \brief Set log file verbosity
     * \param verbosity can be any integer value from enum
     * `AMQPPROX_LOG_LEVELS`
     */
    static void setFileVerbosity(int verbosity);
};

}
}

#endif
