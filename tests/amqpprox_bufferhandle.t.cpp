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
#include <amqpprox_bufferhandle.h>

#include <amqpprox_buffersource.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(BufferHandle, Empty)
{
    BufferHandle handle;
    EXPECT_EQ(handle.data(), nullptr);
    EXPECT_EQ(handle.source(), nullptr);
    EXPECT_EQ(handle.size(), 0);
}

TEST(BufferHandle, Swap)
{
    BufferSource bs1(1);
    BufferSource bs2(2);

    void *       data1 = bs1.acquire();
    void *       data2 = bs2.acquire();
    BufferHandle handle1(data1, 1, &bs1);
    BufferHandle handle2(data2, 2, &bs2);

    handle1.swap(handle2);

    EXPECT_EQ(handle1.data(), data2);
    EXPECT_EQ(handle1.source(), &bs2);
    EXPECT_EQ(handle1.size(), 2);

    EXPECT_EQ(handle2.data(), data1);
    EXPECT_EQ(handle2.source(), &bs1);
    EXPECT_EQ(handle2.size(), 1);
}

TEST(BufferHandle, Explicit_Release)
{
    BufferSource bs1(1);
    void *       data1 = bs1.acquire();
    BufferHandle handle1(data1, 1, &bs1);

    EXPECT_EQ(handle1.data(), data1);
    EXPECT_EQ(handle1.source(), &bs1);
    EXPECT_EQ(handle1.size(), 1);

    handle1.release();

    EXPECT_EQ(handle1.data(), nullptr);
    EXPECT_EQ(handle1.source(), nullptr);
    EXPECT_EQ(handle1.size(), 0);
}

TEST(BufferHandle, Explicit_Assign_From_Empty)
{
    BufferSource bs1(1);
    void *       data1 = bs1.acquire();
    BufferHandle handle1;

    EXPECT_EQ(handle1.data(), nullptr);
    EXPECT_EQ(handle1.source(), nullptr);
    EXPECT_EQ(handle1.size(), 0);

    handle1.assign(data1, 1, &bs1);

    EXPECT_EQ(handle1.data(), data1);
    EXPECT_EQ(handle1.source(), &bs1);
    EXPECT_EQ(handle1.size(), 1);
}

TEST(BufferHandle, Explicit_Assign_From_Filled)
{
    BufferSource bs1(1);
    BufferSource bs2(2);
    void *       data1 = bs1.acquire();
    void *       data2 = bs2.acquire();
    BufferHandle handle1(data2, 2, &bs2);

    EXPECT_EQ(handle1.data(), data2);
    EXPECT_EQ(handle1.source(), &bs2);
    EXPECT_EQ(handle1.size(), 2);

    handle1.assign(data1, 1, &bs1);

    EXPECT_EQ(handle1.data(), data1);
    EXPECT_EQ(handle1.source(), &bs1);
    EXPECT_EQ(handle1.size(), 1);

    // Ensure deallocation happened
    uint64_t allocationCount, deallocationCount, highwaterMark;
    bs2.allocationStats(&allocationCount, &deallocationCount, &highwaterMark);
    EXPECT_EQ(allocationCount, 1);
    EXPECT_EQ(deallocationCount, 1);
}
