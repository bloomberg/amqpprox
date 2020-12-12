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
#ifndef BLOOMBERG_AMQPPROX_TYPES
#define BLOOMBERG_AMQPPROX_TYPES

#include <cstring>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class Buffer;
class FieldArray;
class FieldTable;
class FieldValue;

class Types {
  public:
    static bool decodeLongString(std::string *string, Buffer &buffer);

    static bool decodeShortString(std::string *string, Buffer &buffer);

    static bool encodeLongString(Buffer &buffer, const std::string &string);

    static bool encodeShortString(Buffer &buffer, const std::string &string);

    static bool decodeByteVector(std::vector<uint8_t> *vector,
                                 Buffer &              buffer,
                                 size_t                bytes);

    static bool encodeByteVector(Buffer &                    buffer,
                                 const std::vector<uint8_t> &data);

    static bool decodeFieldValue(FieldValue *value, Buffer &buffer);

    static bool encodeFieldValue(Buffer &buffer, const FieldValue &value);

    static bool decodeFieldArray(std::vector<FieldValue> *vector,
                                 Buffer &                 buffer);

    static bool encodeFieldArray(Buffer &                       buffer,
                                 const std::vector<FieldValue> &data);

    static bool decodeFieldTable(FieldTable *table, Buffer &buffer);

    static bool encodeFieldTable(Buffer &buffer, const FieldTable &table);
};

}
}

#endif
