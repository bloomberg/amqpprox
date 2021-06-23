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

#include <gtest/gtest.h>

using Bloomberg::amqpprox::Buffer;

TEST(Buffer, Breathing)
{
    constexpr static const char buf[] = "HELLO";
    Buffer                      b(buf, 6);
    EXPECT_EQ(b.size(), 6);
}

TEST(Buffer, CopyByte)
{
    constexpr static const char buf[] = "HELLO";
    Buffer                      b(buf, 6);
    EXPECT_EQ(b.size(), 6);
    auto first = b.copy<char>();
    EXPECT_EQ(first, 'H');
    b.skip(3);
    EXPECT_EQ('O', b.copy<char>());
    EXPECT_EQ(b.available(), 1);
}

TEST(Buffer, Equality)
{
    constexpr static const char buf[]  = "HELLO";
    constexpr static const char buf2[] = "HELLO";
    Buffer                      b(buf, 6);
    EXPECT_EQ(b.size(), 6);

    Buffer b2 = b.remaining();

    EXPECT_EQ(b, b2);

    b2.consume(1);
    EXPECT_NE(b, b2);

    Buffer b3 = b2.currentData();
    EXPECT_NE(b, b3);
    EXPECT_NE(b2, b3);

    Buffer b4(buf2, 6);
    EXPECT_NE(b, b4);
}

TEST(Buffer, EqualContents)
{
    constexpr static const char buf[]  = "HELLO";
    constexpr static const char buf2[] = "HELLO";
    Buffer                      b(buf, 6);
    Buffer                      b2(buf2, 6);
    Buffer                      b3(buf2, 5);

    EXPECT_TRUE(b.equalContents(b2));
    EXPECT_FALSE(b.equalContents(b3));
}

TEST(Buffer, Assign)
{
    constexpr static const char buf[] = "HELLO";
    std::vector<char>           target(11);
    Buffer                      dst(target.data(), target.size());
    Buffer                      src(buf, 6);

    bool rc = dst.assign(src);
    dst.skip(src.size());
    EXPECT_TRUE(rc);
    Buffer current = dst.currentData();
    EXPECT_TRUE(current.equalContents(src));

    // Expect the assign to fail
    rc = dst.assign(src);
    EXPECT_FALSE(rc);

    // Also expect writing in the buffer to fail
    EXPECT_FALSE(dst.writeIn(src));

    // Also expect the raw buffer to not be able to write in
    EXPECT_FALSE(dst.writeIn(buf));
}
