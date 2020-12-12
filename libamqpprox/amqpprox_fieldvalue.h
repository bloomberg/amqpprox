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

class FieldValue {
    boost::variant<std::string,
                   uint64_t,
                   int64_t,
                   double,
                   bool,
                   std::vector<uint8_t>,
                   std::vector<FieldValue>,
                   std::shared_ptr<FieldTable>>
         d_value;
    char d_type;

  public:
    FieldValue(char type, const std::string &value);
    FieldValue(char type, int64_t value);
    FieldValue(char type, uint64_t value);
    FieldValue(char type, double value);
    FieldValue(char type, bool value);
    FieldValue(char type, std::vector<uint8_t> value);
    FieldValue(char type, std::vector<FieldValue> value);
    FieldValue(char type, const std::shared_ptr<FieldTable> &value);

    inline char type() const;

    template <typename T>
    T value();

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
