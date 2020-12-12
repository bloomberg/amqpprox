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
#include <amqpprox_fieldvalue.h>

#include <amqpprox_fieldtable.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

FieldValue::FieldValue(char type, const std::string &value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, int64_t value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, uint64_t value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, double value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, bool value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, std::vector<uint8_t> value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, std::vector<FieldValue> value)
: d_value(value)
, d_type(type)
{
}

FieldValue::FieldValue(char type, const std::shared_ptr<FieldTable> &value)
: d_value(value)
, d_type(type)
{
}

std::ostream &operator<<(std::ostream &os, const FieldValue &value)
{
    switch (value.d_type) {
    case 'F':
        os << *boost::get<std::shared_ptr<FieldTable>>(value.d_value);
        break;
    case 'S':
    case 's':
        os << "\"";
        boost::apply_visitor(FieldValuePrinter(os), value.d_value);
        os << "\"";
        break;
    default:
        boost::apply_visitor(FieldValuePrinter(os), value.d_value);
        break;
    }
    return os;
}

template <>
std::ostream &FieldValuePrinter::operator()(std::vector<uint8_t> value) const
{
    d_printStream << "[";
    for (std::size_t i = 0, l = value.size(); i < l; ++i) {
        d_printStream << static_cast<uint32_t>(value[i]);
        if (i < l - 1) {
            d_printStream << ", ";
        }
    }
    d_printStream << "]";
    return d_printStream;
}

template <>
std::ostream &
FieldValuePrinter::operator()(std::vector<FieldValue> value) const
{
    d_printStream << "[";
    for (std::size_t i = 0, l = value.size(); i < l; ++i) {
        d_printStream << value[i];
        if (i < l - 1) {
            d_printStream << ", ";
        }
    }
    d_printStream << "]";
    return d_printStream;
}

}
}
