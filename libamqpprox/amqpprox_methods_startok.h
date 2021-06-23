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
#ifndef BLOOMBERG_AMQPPROX_METHODS_STARTOK
#define BLOOMBERG_AMQPPROX_METHODS_STARTOK

#include <amqpprox_fieldtable.h>

#include <boost/endian/arithmetic.hpp>
#include <iosfwd>
#include <string>

namespace Bloomberg {
namespace amqpprox {

class Buffer;

namespace methods {

class StartOk {
    FieldTable  d_properties;
    std::string d_mechanism;
    std::string d_response;
    std::string d_locale;

  public:
    const FieldTable &properties() const { return d_properties; }

    FieldTable &properties() { return d_properties; }

    const std::string &mechanism() const { return d_mechanism; }

    const std::string &response() const { return d_response; }

    const std::string &locale() const { return d_locale; }

    static bool decode(StartOk *startOk, Buffer &buffer);

    static bool encode(Buffer &buffer, const StartOk &start);

    constexpr inline static int classType() { return 10; }

    constexpr inline static int methodType() { return 11; }
};

std::ostream &operator<<(std::ostream &os, const StartOk &okMethod);

}
}
}

#endif
