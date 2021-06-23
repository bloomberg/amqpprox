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
#include <amqpprox_eventsourcesignal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;

TEST(EventSourceSignal, Breathing)
{
    auto signal = EventSourceSignal<int, int>::create();
    signal->emit(0, 1);
}

TEST(EventSourceSignal, FiresTwice)
{
    int first  = 0;
    int second = 0;
    int third  = 0;

    auto signal = EventSourceSignal<int, int>::create();
    auto subs1  = signal->subscribe([&first](int a, int b) { first = a + b; });

    auto subs2 =
        signal->subscribe([&second](int a, int b) { second = a - b; });

    signal->emit(100, 50);

    EXPECT_EQ(first, 150);
    EXPECT_EQ(second, 50);
    EXPECT_EQ(third, 0);

    subs1.release();
    signal->emit(1000, 501);

    EXPECT_EQ(first, 150);
    EXPECT_EQ(second, 499);
    EXPECT_EQ(third, 0);

    // Test automatic desubscription
    {
        auto subs3 =
            signal->subscribe([&third](int a, int b) { third = a * b; });
        signal->emit(2, 7);

        EXPECT_EQ(first, 150);
        EXPECT_EQ(second, -5);
        EXPECT_EQ(third, 14);
    }

    signal->emit(10, 1);

    EXPECT_EQ(first, 150);
    EXPECT_EQ(second, 9);
    EXPECT_EQ(third, 14);
}
