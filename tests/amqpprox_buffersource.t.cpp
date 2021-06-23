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
#include <amqpprox_buffersource.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(BufferSource, Breathing)
{
    BufferSource bs(100);

    EXPECT_EQ(bs.bufferSize(), 100);

    uint64_t allocationCount, deallocationCount, highwaterMark;
    bs.allocationStats(&allocationCount, &deallocationCount, &highwaterMark);
    EXPECT_EQ(allocationCount, 0);
    EXPECT_EQ(deallocationCount, 0);
    EXPECT_EQ(highwaterMark, 0);
}

TEST(BufferSource, SimpleAllocationsStats)
{
    uint64_t     allocationCount, deallocationCount, highwaterMark;
    BufferSource bs(100);

    auto buf1 = bs.acquire();
    auto buf2 = bs.acquire();
    auto buf3 = bs.acquire();
    bs.release(buf3);

    bs.allocationStats(&allocationCount, &deallocationCount, &highwaterMark);
    EXPECT_EQ(allocationCount, 3);
    EXPECT_EQ(deallocationCount, 1);
    EXPECT_EQ(highwaterMark, 3);

    bs.release(buf1);
    bs.release(buf2);
    auto buf4 = bs.acquire();
    bs.release(buf4);

    bs.allocationStats(&allocationCount, &deallocationCount, &highwaterMark);
    EXPECT_EQ(allocationCount, 4);
    EXPECT_EQ(deallocationCount, 4);
    EXPECT_EQ(highwaterMark, 3);
}

TEST(BufferSource, Use_Provided_Buffer)
{
    BufferSource bs(4096);

    auto buf1 = bs.acquire();
    auto buf2 = bs.acquire();
    memset(buf1, 'F', 4096);
    memset(buf2, 'F', 4096);
    EXPECT_EQ(memcmp(buf1, buf2, 4096), 0);
}
