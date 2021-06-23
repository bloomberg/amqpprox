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
#ifndef BLOOMBERG_AMQPPROX_FIELDTABLE
#define BLOOMBERG_AMQPPROX_FIELDTABLE

#include <amqpprox_fieldvalue.h>

#include <iosfwd>
#include <string>
#include <vector>

namespace Bloomberg {
namespace amqpprox {

class FieldTable {
    std::vector<std::pair<std::string, FieldValue>> d_fields;

  public:
    FieldTable();

    void pushField(const std::string &name, const FieldValue &value);
    void reset();

    bool findFieldValue(FieldValue *value, const std::string &name) const;
    bool findFieldIndex(std::size_t *index, const std::string &name) const;
    std::size_t        numberFields() const;
    const FieldValue & fieldIndex(std::size_t index) const;
    FieldValue &       fieldIndex(std::size_t index);
    const std::string &fieldName(std::size_t index) const;
    std::string &      fieldName(std::size_t index);
};

inline void FieldTable::pushField(const std::string &name,
                                  const FieldValue & value)
{
    d_fields.emplace_back(std::make_pair(name, value));
}

inline void FieldTable::reset()
{
    d_fields.clear();
}

inline bool FieldTable::findFieldValue(FieldValue *       value,
                                       const std::string &name) const
{
    for (const auto &vals : d_fields) {
        if (name == vals.first) {
            *value = vals.second;
            return true;
        }
    }
    return false;
}

inline bool FieldTable::findFieldIndex(std::size_t *      index,
                                       const std::string &name) const
{
    int i = 0;
    for (const auto &vals : d_fields) {
        if (name == vals.first) {
            *index = i;
            return true;
        }
        ++i;
    }
    return false;
}

inline std::size_t FieldTable::numberFields() const
{
    return d_fields.size();
}

inline const FieldValue &FieldTable::fieldIndex(std::size_t index) const
{
    return d_fields[index].second;
}

inline FieldValue &FieldTable::fieldIndex(std::size_t index)
{
    return d_fields[index].second;
}

inline const std::string &FieldTable::fieldName(std::size_t index) const
{
    return d_fields[index].first;
}

inline std::string &FieldTable::fieldName(std::size_t index)
{
    return d_fields[index].first;
}

std::ostream &operator<<(std::ostream &os, const FieldTable &table);
std::ostream &operator<<(std::ostream &                     os,
                         const std::shared_ptr<FieldTable> &table);

bool        operator==(const FieldTable &lhs, const FieldTable &rhs);
inline bool operator!=(const FieldTable &lhs, const FieldTable &rhs)
{
    return !(lhs == rhs);
}

}
}

#endif
