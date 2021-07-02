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
#ifndef BLOOMBERG_AMQPPROX_FIELDVALUE
#define BLOOMBERG_AMQPPROX_FIELDVALUE

#include <boost/variant.hpp>

#include <iosfwd>
#include <memory>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class FieldTable;

/**
 * \brief Represents a RabbitMQ AMQP Field type
 * https://www.rabbitmq.com/amqp-0-9-1-errata.html
 */
class FieldValue {
    boost::variant<std::string,
                   uint64_t,
                   int64_t,
                   bool,
                   std::vector<uint8_t>,
                   std::vector<FieldValue>,
                   std::shared_ptr<FieldTable>>
         d_value;
    char d_type;

  public:
    /**
     * \brief create a string value type field
     * \param type the RabbitMQ type, this should be 'S' for Long String
     * \param value the string value
     */
    FieldValue(char type, const std::string &value);

    /**
     * \brief create a signed integer value type field
     * \param type the RabbitMQ type, this should be 'b','s','I','l' for 8, 16,
     * 32, 64 bit signed integers respectively
     * \param value the signed integer value, note that passing a value higher
     * than can be represented by the RabbitMQ `type` yields undefined
     * behaviour.
     */
    FieldValue(char type, int64_t value);

    /**
     * \brief create a boolean or empty value type field
     * \param type the RabbitMQ type, this should be 't', or 'V' for empty
     * field
     * \param value of the boolean
     */
    FieldValue(char type, bool value);

    /**
     * \brief create a byte array style value type field
     * \param type the RabbitMQ type, this should be 'f','d','D','x'
     * for 32bit float, 64bit float (double), decimal, byte array respectively
     * \param value expressed as bytes in host byte order.
     */
    FieldValue(char type, std::vector<uint8_t> value);

    /**
     * \brief create a Field Array value
     * \param type the RabbitMQ type, this should be 'A'
     * \param value of the field array
     */
    FieldValue(char type, std::vector<FieldValue> value);

    /**
     * \brief create a Field Table value
     * \param type the RabbitMQ type, this should be 'F'
     * \param value shared pointer pointing to the field table
     */
    FieldValue(char type, const std::shared_ptr<FieldTable> &value);

    /**
     * \brief create a signed integer value type field
     * \param type the RabbitMQ type, this should be 'B','u','i'
     * 8, 16, 32 bit unsigned integers respectively
     * \param value the unsigned integer value, note that passing a value
     * higher than can be represented by the RabbitMQ `type` yields undefined
     * behaviour.
     */
    FieldValue(char type, uint64_t value);

    /**
     * \returns the amqp type enumeration of the field e.g. 't' for boolean,
     * 'D' for decimal
     */
    inline char type() const;

    /**
     * \returns the value of the field provided the typename specified is
     * complaint with what type is set on the variant, otherwise undefined
     * behaviour.
     */
    template <typename T>
    T value();

    /**
     * \returns the value of the field provided the typename specified is
     * complaint with what type is set on the variant, otherwise undefined
     * behaviour.
     */
    template <typename T>
    T value() const;

    friend std::ostream &operator<<(std::ostream &os, const FieldValue &value);
    friend bool operator==(const FieldValue &lhs, const FieldValue &rhs);
};

inline char FieldValue::type() const
{
    return d_type;
}

template <typename T>
T FieldValue::value()
{
    return boost::get<T>(d_value);
}

template <typename T>
T FieldValue::value() const
{
    return boost::get<T>(d_value);
}

std::ostream &operator<<(std::ostream &os, const FieldValue &value);

inline bool operator==(const FieldValue &lhs, const FieldValue &rhs)
{
    return lhs.type() == rhs.type() && lhs.d_value == rhs.d_value;
}

inline bool operator!=(const FieldValue &lhs, const FieldValue &rhs)
{
    return !(lhs == rhs);
}

/**
 * \brief Helper class to print AMQP field types on output stream
 */
class FieldValuePrinter : public boost::static_visitor<std::ostream &> {
    std::ostream &d_printStream;

  public:
    FieldValuePrinter(std::ostream &stream)
    : d_printStream(stream)
    {
    }

    template <typename T>
    std::ostream &operator()(T value) const;
};

template <typename T>
std::ostream &FieldValuePrinter::operator()(T value) const
{
    d_printStream << value;
    return d_printStream;
}

}
}

#endif
