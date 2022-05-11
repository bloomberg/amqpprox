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

/**
 * \brief Helper class to encode/decode AMQP 0.9.1 data types.
 *
 * These types are used in AMQP methods framing. Note this is not 'pure' AMQP
 * 0-9-1 aiming for the same compatibility as the rabbitmq server
 * implementation see: https://www.rabbitmq.com/amqp-0-9-1-errata.html and
 * https://github.com/rabbitmq/rabbitmq-server/blob/master/deps/rabbit_common/src/rabbit_binary_parser.erl
 */
class Types {
  public:
    /**
     * \brief Decode the specified buffer and convert the data into AMQP long
     * string type
     * \param string AMQP long string stored as std::string
     * \param buffer raw data stored as `Buffer`
     * \return true in case of success, otherwise false
     */
    static bool decodeLongString(std::string *string, Buffer &buffer);

    /**
     * \brief Decode the specified buffer and convert the data into AMQP short
     * string type
     * \param string AMQP short string stored as std::string
     * \param buffer raw data stored as `Buffer`
     * \return true in case of success, otherwise false
     */
    static bool decodeShortString(std::string *string, Buffer &buffer);

    /**
     * \brief Encode AMQP long string and write the data into the specified
     * buffer
     * \param buffer raw data stored as `Buffer`
     * \param string AMQP long string stored as std::string
     * \return true in case of success, otherwise false
     */
    static bool encodeLongString(Buffer &buffer, const std::string &string);

    /**
     * \brief Encode AMQP short string and write the data into the specified
     * buffer
     * \param buffer raw data stored as `Buffer`
     * \param string AMQP short string stored as std::string
     * \return true in case of success, otherwise false
     */
    static bool encodeShortString(Buffer &buffer, const std::string &string);

    /**
     * \brief Decode the specified buffer and convert the data into AMQP byte
     * array type
     * \param vector AMQP byte array type stored as std::vector of unit8_t type
     * \param buffer raw data stored as `Buffer`
     * \param bytes amount of bytes to be considered from the buffer
     * \return true in case of success, otherwise false
     */
    static bool decodeByteVector(std::vector<uint8_t> *vector,
                                 Buffer               &buffer,
                                 size_t                bytes);

    /**
     * \brief Encode AMQP byte array and write the data into the specified
     * buffer
     * \param buffer raw data stored as `Buffer`
     * \param data AMQP byte array type stored as std::vector of unit8_t type
     * \return true in case of success, otherwise false
     */
    static bool encodeByteVector(Buffer                     &buffer,
                                 const std::vector<uint8_t> &data);

    /**
     * \brief Decode the specified buffer and convert the data into AMQP field
     * type
     * \param value AMQP field type stored as `FieldValue`
     * \param buffer raw data stored as `Buffer`
     * \return true in case of success, otherwise false
     */
    static bool decodeFieldValue(FieldValue *value, Buffer &buffer);

    /**
     * \brief Encode AMQP field type and write the data into the specified
     * buffer
     * \param buffer raw data stored as `Buffer`
     * \param value AMQP field type stored as `FieldValue`
     * \return true in case of success, otherwise false
     */
    static bool encodeFieldValue(Buffer &buffer, const FieldValue &value);

    /**
     * \brief Decode the specified buffer and convert the data into AMQP field
     * type array
     * \param vector AMQP field type array stored as std::vector of
     * `FieldValue`
     * \param buffer raw data stored as `Buffer`
     * \return true in case of success, otherwise false
     */
    static bool decodeFieldArray(std::vector<FieldValue> *vector,
                                 Buffer                  &buffer);

    /**
     * \brief Encode AMQP field type array and write the data into the
     * specified buffer
     * \param buffer raw data stored as `Buffer`
     * \param data AMQP field type array stored as std::vector of `FieldValue`
     * \return true in case of success, otherwise false
     */
    static bool encodeFieldArray(Buffer                        &buffer,
                                 const std::vector<FieldValue> &data);

    /**
     * \brief Decode the specified buffer and convert the data into AMQP field
     * table type
     * \param table AMQP field table type stored as `FieldTable`
     * \param buffer raw data stored as `Buffer`
     * \return true in case of success, otherwise false
     */
    static bool decodeFieldTable(FieldTable *table, Buffer &buffer);

    /**
     * \brief Encode AMQP field table and write the data into the specified
     * buffer
     * \param buffer raw data stored as `Buffer`
     * \param table AMQP field table type stored as `FieldTable`
     * \return true in case of success, otherwise false
     */
    static bool encodeFieldTable(Buffer &buffer, const FieldTable &table);
};

}
}

#endif
