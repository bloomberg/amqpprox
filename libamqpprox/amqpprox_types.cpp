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
#include <amqpprox_types.h>

#include <amqpprox_buffer.h>
#include <amqpprox_constants.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_fieldvalue.h>
#include <amqpprox_logging.h>

#include <boost/endian/arithmetic.hpp>

#include <cstdint>
#include <vector>

// Note this is not 'pure' AMQP 0-9-1  aiming for the same compatibility as the
// server see https://www.rabbitmq.com/amqp-0-9-1-errata.html &
// https://github.com/rabbitmq/rabbitmq-server/blob/master/deps/rabbit_common/src/rabbit_binary_parser.erl

namespace Bloomberg {
namespace amqpprox {

namespace {

const int FLOAT_OCTETS   = 4;
const int DOUBLE_OCTETS  = 8;
const int DECIMAL_OCTETS = 5;

template <typename T>
bool decodeString(std::string *string, Buffer &buffer)
{
    assert(string != nullptr);
    if (buffer.available() < sizeof(T)) {
        return false;
    }

    auto stringLength = buffer.copy<T>();

    if (stringLength > buffer.available()) {
        return false;
    }

    auto stringBuffer = buffer.consume(stringLength);
    string->assign(static_cast<const char *>(stringBuffer.ptr()),
                   stringBuffer.size());
    return true;
}

}

using namespace boost::endian;

bool Types::decodeLongString(std::string *string, Buffer &buffer)
{
    return decodeString<big_uint32_t>(string, buffer);
}

bool Types::decodeShortString(std::string *string, Buffer &buffer)
{
    return decodeString<big_uint8_t>(string, buffer);
}

bool Types::encodeLongString(Buffer &buffer, const std::string &string)
{
    if (buffer.available() < (string.size() + sizeof(big_uint32_t))) {
        return false;
    }

    buffer.writeIn<boost::endian::big_uint32_t>(string.size());
    buffer.writeIn(Buffer(string.data(), string.size()));
    return true;
}

bool Types::encodeShortString(Buffer &buffer, const std::string &string)
{
    if (buffer.available() < (string.size() + sizeof(big_uint8_t))) {
        return false;
    }

    if (string.size() > Constants::shortStringLimit()) {
        return false;
    }

    buffer.writeIn<boost::endian::big_uint8_t>(string.size());
    buffer.writeIn(Buffer(string.data(), string.size()));

    return true;
}

bool Types::decodeByteVector(std::vector<uint8_t> *vector,
                             Buffer               &buffer,
                             size_t                bytes)
{
    assert(vector != nullptr);

    if (bytes > buffer.available()) {
        return false;
    }

    auto vectorBuffer = buffer.consume(bytes);

    vector->resize(vectorBuffer.size());
    memcpy(vector->data(), vectorBuffer.ptr(), vectorBuffer.size());

    return true;
}

bool Types::encodeByteVector(Buffer                     &buffer,
                             const std::vector<uint8_t> &vector)
{
    if (buffer.available() < (vector.size())) {
        return false;
    }

    buffer.writeIn(Buffer(vector.data(), vector.size()));
    return true;
}

bool Types::decodeFieldValue(FieldValue *outValue, Buffer &buffer)
{
    assert(outValue != nullptr);

    char type = buffer.copy<char>();
    switch (type) {
    case 't':  // boolean
    {
        FieldValue value(type, (buffer.copy<char>() != 0));
        *outValue = value;
    } break;
    case 'b':  // short-short-int
    {
        FieldValue value(type, (int64_t)buffer.copy<big_int8_t>());
        *outValue = value;
    } break;
    case 'B':  // short-short-uint
    {
        FieldValue value(type, (uint64_t)buffer.copy<big_uint8_t>());
        *outValue = value;
    } break;
    case 'U':
        LOG_DEBUG << "Converting unsupported field type 'U' to 's'";
        // fall through
    case 's':  // short-int
    {
        FieldValue value('s', (int64_t)buffer.copy<big_int16_t>());
        *outValue = value;
    } break;
    case 'u':  // short-uint
    {
        FieldValue value(type, (uint64_t)buffer.copy<big_uint16_t>());
        *outValue = value;
    } break;
    case 'I':  // long-int
    {
        FieldValue value(type, (int64_t)buffer.copy<big_int32_t>());
        *outValue = value;
    } break;
    case 'i':  // long-uint
    {
        FieldValue value(type, (uint64_t)buffer.copy<big_uint32_t>());
        *outValue = value;
    } break;
    case 'l':  // long-long-int
    case 'L':  // compatibility long-long-int
    {
        FieldValue value(type, (int64_t)buffer.copy<big_int64_t>());
        *outValue = value;
    } break;
    case 'f':  // float
    {
        std::vector<uint8_t> floatBuf;
        if (!decodeByteVector(&floatBuf, buffer, FLOAT_OCTETS)) {
            return false;
        }
        FieldValue value(type, floatBuf);
        *outValue = value;
    } break;
    case 'd':  // double
    {
        std::vector<uint8_t> doubleBuf;
        if (!decodeByteVector(&doubleBuf, buffer, DOUBLE_OCTETS)) {
            return false;
        }
        FieldValue value(type, doubleBuf);
        *outValue = value;
    } break;
    case 'D':  // decimal-value
    {
        std::vector<uint8_t> decimalBuf;
        if (!decodeByteVector(&decimalBuf, buffer, DECIMAL_OCTETS)) {
            return false;
        }
        FieldValue value(type, decimalBuf);
        *outValue = value;
    } break;
    case 'S':  // long-string
    {
        std::string val;
        if (!decodeLongString(&val, buffer)) {
            return false;
        }
        FieldValue value(type, val);
        *outValue = value;
    } break;
    case 'A':  // field-array
    {
        std::vector<FieldValue> val;
        if (!decodeFieldArray(&val, buffer)) {
            return false;
        }
        FieldValue value(type, val);
        *outValue = value;
    } break;
    case 'T':  // timestamp
    {
        FieldValue value(type, (uint64_t)buffer.copy<big_uint64_t>());
        *outValue = value;
    } break;
    case 'F':  // field-table
    {
        auto sp = std::make_shared<FieldTable>();
        if (!decodeFieldTable(sp.get(), buffer)) {
            return false;
        }
        FieldValue value(type, sp);
        *outValue = value;
    } break;
    case 'V':  // No value
    {
        // There is no value to return.
    } break;
    case 'x':  // byte array
    {
        std::vector<uint8_t> val;
        if (buffer.available() < sizeof(uint32_t)) {
            return false;
        }

        uint32_t length = buffer.copy<big_uint32_t>();

        if (!decodeByteVector(&val, buffer, length)) {
            return false;
        }
        FieldValue value(type, val);
        *outValue = value;
    } break;
    default: {
        return false;
    }
    }

    return true;
}

bool Types::encodeFieldValue(Buffer &buffer, const FieldValue &fv)
{
    Buffer writeBuffer = buffer.remaining();
    if (!writeBuffer.writeIn<big_uint8_t>(fv.type())) {
        return false;
    }
    switch (fv.type()) {
    case 't':  // boolean
    {
        bool value   = fv.value<bool>();
        bool success = false;
        if (value) {
            success = writeBuffer.writeIn<big_uint8_t>(1);
        }
        else {
            success = writeBuffer.writeIn<big_uint8_t>(0);
        }
        if (!success) {
            return false;
        }
    } break;
    case 'b':  // short-short-int
    {
        if (!writeBuffer.writeIn<big_int8_t>(fv.value<int64_t>())) {
            return false;
        }
    } break;
    case 'B':  // short-short-uint
    {
        if (!writeBuffer.writeIn<big_uint8_t>(fv.value<uint64_t>())) {
            return false;
        }
    } break;
    case 's':  // short-int
    {
        if (!writeBuffer.writeIn<big_int16_t>(fv.value<int64_t>())) {
            return false;
        }
    } break;
    case 'u':  // short-uint
    {
        if (!writeBuffer.writeIn<big_uint16_t>(fv.value<uint64_t>())) {
            return false;
        }
    } break;
    case 'I':  // long-int
    {
        if (!writeBuffer.writeIn<big_int32_t>(fv.value<int64_t>())) {
            return false;
        }
    } break;
    case 'i':  // long-uint
    {
        if (!writeBuffer.writeIn<big_uint32_t>(fv.value<uint64_t>())) {
            return false;
        }
    } break;
    case 'L':  // compatibility long-long-int
    case 'l':  // long-long-int
    {
        if (!writeBuffer.writeIn<big_int64_t>(fv.value<int64_t>())) {
            return false;
        }
    } break;
    case 'f':  // float
    case 'd':  // double
    case 'D':  // decimal-value
    {
        if (!encodeByteVector(writeBuffer, fv.value<std::vector<uint8_t>>())) {
            return false;
        }
    } break;
    case 'S':  // long-string
    {
        if (!encodeLongString(writeBuffer, fv.value<std::string>())) {
            return false;
        }
    } break;
    case 'A':  // field-array
    {
        Buffer spaceLeft = writeBuffer.remaining();
        if (!encodeFieldArray(spaceLeft,
                              fv.value<std::vector<FieldValue>>())) {
            return false;
        }
        writeBuffer.skip(spaceLeft.offset());
    } break;
    case 'T':  // timestamp
    {
        if (!writeBuffer.writeIn<big_uint64_t>(fv.value<uint64_t>())) {
            return false;
        }
    } break;
    case 'F':  // field-table
    {
        Buffer spaceLeft = writeBuffer.remaining();
        if (!encodeFieldTable(spaceLeft,
                              *fv.value<std::shared_ptr<FieldTable>>())) {
            return false;
        }
        writeBuffer.skip(spaceLeft.offset());
    } break;
    case 'V':  // No value
    {
        // There is no value to write
    } break;
    case 'x':  // byte array
    {
        const std::vector<uint8_t> &byteArray =
            fv.value<std::vector<uint8_t>>();
        if (!writeBuffer.writeIn<big_uint32_t>(byteArray.size())) {
            return false;
        }
        if (!encodeByteVector(writeBuffer, byteArray)) {
            return false;
        }
    } break;
    default: {
        return false;
    }
    }
    buffer.skip(writeBuffer.offset());

    return true;
}

bool Types::decodeFieldArray(std::vector<FieldValue> *vector, Buffer &buffer)
{
    assert(vector != nullptr);

    if (sizeof(big_uint32_t) > buffer.available()) {
        return false;
    }

    auto arrayLength = buffer.copy<big_uint32_t>();
    if (arrayLength > buffer.available()) {
        return false;
    }

    auto arrayBuffer = buffer.consume(arrayLength);
    while (arrayBuffer.available() > 0) {
        FieldValue value('V', false);
        if (!decodeFieldValue(&value, arrayBuffer)) {
            return false;
        }

        vector->push_back(value);
    }

    return true;
}

bool Types::encodeFieldArray(Buffer                        &buffer,
                             const std::vector<FieldValue> &vector)
{
    Buffer writeBuffer = buffer.remaining();
    writeBuffer.skip(sizeof(big_uint32_t));
    std::size_t originalOffset = writeBuffer.offset();

    for (auto i = 0u; i < vector.size(); ++i) {
        if (!encodeFieldValue(writeBuffer, vector[i])) {
            return false;
        }
    }

    std::size_t endOffset = writeBuffer.offset();
    std::size_t length    = endOffset - originalOffset;
    writeBuffer.seek(0);
    writeBuffer.writeIn<big_uint32_t>(length);
    buffer.skip(endOffset);
    return true;
}

bool Types::decodeFieldTable(FieldTable *table, Buffer &buffer)
{
    if (sizeof(big_uint32_t) > buffer.available()) {
        return false;
    }

    auto fieldTableLength = buffer.copy<big_uint32_t>();
    if (fieldTableLength > buffer.available()) {
        return false;
    }

    Buffer tBuffer = buffer.consume(fieldTableLength);
    while (tBuffer.available() > 0) {
        std::string fieldName;
        if (!decodeShortString(&fieldName, tBuffer)) {
            return false;
        }

        FieldValue value('V', false);
        if (!decodeFieldValue(&value, tBuffer)) {
            return false;
        }

        table->pushField(fieldName, value);
    }
    return true;
}

bool Types::encodeFieldTable(Buffer &buffer, const FieldTable &table)
{
    Buffer writeBuffer = buffer.remaining();
    writeBuffer.skip(sizeof(big_uint32_t));
    std::size_t originalOffset = writeBuffer.offset();

    for (auto i = 0u; i < table.numberFields(); ++i) {
        if (!encodeShortString(writeBuffer, table.fieldName(i))) {
            return false;
        }

        auto fv = table.fieldIndex(i);
        if (!encodeFieldValue(writeBuffer, fv)) {
            return false;
        }
    }
    std::size_t endOffset = writeBuffer.offset();
    std::size_t length    = endOffset - originalOffset;
    writeBuffer.seek(0);
    writeBuffer.writeIn<big_uint32_t>(length);
    buffer.skip(endOffset);
    return true;
}

}
}
