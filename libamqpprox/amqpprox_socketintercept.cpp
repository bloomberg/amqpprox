/*
** Copyright 2021 Bloomberg Finance L.P.
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

#include <amqpprox_socketintercept.h>
#include <stdexcept>

namespace Bloomberg {
namespace amqpprox {

void SocketIntercept::setSecure(bool secure)
{
    d_impl.setSecure(secure);
}

bool SocketIntercept::isSecure() const
{
    return d_impl.isSecure();
}

void SocketIntercept::refreshSocketContext()
{
    d_impl.refreshSocketContext();
}

SocketIntercept::endpoint
SocketIntercept::remote_endpoint(boost::system::error_code &ec)
{
    return d_impl.remote_endpoint(ec);
}

SocketIntercept::endpoint
SocketIntercept::local_endpoint(boost::system::error_code &ec)
{
    return d_impl.local_endpoint(ec);
}

void SocketIntercept::shutdown(boost::system::error_code &ec)
{
    d_impl.shutdown(ec);
}

void SocketIntercept::close(boost::system::error_code &ec)
{
    d_impl.close(ec);
}

void SocketIntercept::setDefaultOptions(boost::system::error_code &ec)
{
    d_impl.setDefaultOptions(ec);
}

std::size_t SocketIntercept::available(boost::system::error_code &ec)
{
    return d_impl.available(ec);
}

}
}
