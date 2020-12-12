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

#include <amqpprox_vhoststate.h>

#include <gtest/gtest.h>
#include <sstream>

using Bloomberg::amqpprox::VhostState;

TEST(VhostState, Starts_Unpaused) {
    VhostState state;
    EXPECT_FALSE(state.isPaused("/"));
}

TEST(VhostState, Manipulate) {
    VhostState state;

    // Manipulate to true
    state.setPaused("/", true);
    EXPECT_TRUE(state.isPaused("/"));
    
    // Check unrelated
    EXPECT_FALSE(state.isPaused("unrelated"));

    // Manipulate to false
    state.setPaused("/", false);
    EXPECT_FALSE(state.isPaused("/"));
    
    // Check unrelated
    EXPECT_FALSE(state.isPaused("unrelated"));
}

TEST(VhostState, Print_Empty) {
    VhostState state;

    std::ostringstream oss;
    state.print(oss);

    EXPECT_EQ(oss.str(), "");
}

TEST(VhostState, Print_Two) {
    VhostState state;
    state.setPaused("foo", true);
    state.setPaused("bar", false);

    std::ostringstream oss;
    state.print(oss);

    EXPECT_EQ(oss.str(), "bar = UNPAUSED\nfoo = PAUSED\n");
}
