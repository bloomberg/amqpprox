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
#include <amqpprox_fieldtable.h>

#include <iostream>

namespace Bloomberg {
namespace amqpprox {

FieldTable::FieldTable()
: d_fields()
{
}

std::ostream &operator<<(std::ostream &os, const FieldTable &table)
{
    os << "[";
    auto len = table.numberFields();
    for (auto i = 0u; i < len; ++i) {
        if (0 != i) {
            os << ", ";
        }
        os << table.fieldName(i) << ": " << table.fieldIndex(i);
    }
    os << "]";
    return os;
}

std::ostream &operator<<(std::ostream                      &os,
                         const std::shared_ptr<FieldTable> &table)
{
    return os << *table;
}

bool operator==(const FieldTable &lhs, const FieldTable &rhs)
{
    if (lhs.numberFields() != rhs.numberFields()) {
        return false;
    }

    for (auto i = 0u; i < lhs.numberFields(); ++i) {
        if (lhs.fieldName(i) != rhs.fieldName(i) ||
            lhs.fieldIndex(i) != rhs.fieldIndex(i)) {
            return false;
        }
    }

    return true;
}

}
}
