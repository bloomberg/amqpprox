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
#include <amqpprox_buffer.h>

namespace Bloomberg {
namespace amqpprox {

bool Buffer::equalContents(const Buffer &rhs) const
{
    if (size() != rhs.size()) {
        return false;
    }
    return 0 == memcmp(originalPtr(), rhs.originalPtr(), size());
}

bool operator==(const Buffer &b1, const Buffer &b2)
{
    if (b1.originalPtr() != b2.originalPtr()) {
        return false;
    }
    if (b1.size() != b2.size()) {
        return false;
    }
    if (b1.ptr() != b2.ptr()) {
        return false;
    }

    return true;
}

bool operator!=(const Buffer &b1, const Buffer &b2)
{
    return !(b1 == b2);
}

}
}
