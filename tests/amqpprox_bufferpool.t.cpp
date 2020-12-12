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
#include <amqpprox_bufferpool.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(BufferPool, Breathing)
{
    BufferPool bp({});
}

TEST(BufferPool, Multiple_Buffer_Sizes_Out_Of_Order)
{
    BufferPool bp({200, 100});
}

TEST(BufferPool, Different_Sizes_Different_Sources)
{
    BufferPool bp({2, 1});

    BufferHandle handle1;
    bp.acquireBuffer(&handle1, 1);

    BufferHandle handle2;
    bp.acquireBuffer(&handle2, 2);

    BufferHandle handle3;
    bp.acquireBuffer(&handle3, 3);

    EXPECT_NE(handle1.source(), handle2.source());
    EXPECT_EQ(handle3.source(), nullptr);
    EXPECT_EQ(handle1.size(), 1);
    EXPECT_EQ(handle2.size(), 2);
    EXPECT_EQ(handle3.size(), 3);

    handle2.release();

    uint64_t                                      spillCount = 0;
    std::vector<BufferPool::BufferAllocationStat> pstats;
    bp.getPoolStatistics(&pstats, &spillCount);
    EXPECT_EQ(spillCount, 1);
    EXPECT_EQ(pstats[0], std::make_tuple(1ull, 1ull, 1ull));
    EXPECT_EQ(pstats[1], std::make_tuple(2ull, 0ull, 1ull));
}
