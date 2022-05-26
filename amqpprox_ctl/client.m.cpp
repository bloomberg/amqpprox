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
#include <boost/asio.hpp>

#include <iostream>

using boost::asio::local::stream_protocol;

static const int REPLY_BUFFER_SIZE = 1024;

int main(int argc, char *argv[])
{
    try {
        if (argc <= 2) {
            std::cerr << "Usage: amqpprox_ctl <control_socket> ARGS\n";
            return 1;
        }

        boost::asio::io_context ioContext;

        stream_protocol::socket clientSocket(ioContext);
        clientSocket.connect(stream_protocol::endpoint(argv[1]));

        std::string allArgs{argv[2]};
        for (int i = 3; i < argc; ++i) {
            std::string arg(argv[i]);
            allArgs += " " + arg;
        }
        allArgs += "\n";

        boost::asio::write(
            clientSocket,
            boost::asio::buffer(allArgs.data(), allArgs.length()));

        boost::system::error_code ec;
        char                      reply[REPLY_BUFFER_SIZE];
        while (true) {
            size_t replyLength = clientSocket.read_some(
                boost::asio::buffer(reply, REPLY_BUFFER_SIZE), ec);

            if (replyLength == 0 || ec) {
                break;
            }

            std::cout.write(reply, replyLength);
            std::cout.flush();
        }
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 2;
    }

    return 0;
}
