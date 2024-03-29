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

#include <amqpprox_backendselector.h>
#include <amqpprox_backendselectorstore.h>
#include <amqpprox_robinbackendselector.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Bloomberg;
using namespace amqpprox;
using namespace testing;

TEST(BackendSelectorStore, Breathing)
{
    BackendSelectorStore             store;
    std::unique_ptr<BackendSelector> selector(new RobinBackendSelector());
    store.addSelector(std::move(selector));

    EXPECT_EQ(store.getSelector("not-existing"), nullptr);
    EXPECT_NE(store.getSelector("round-robin"), nullptr);
}
