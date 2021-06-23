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
#include <amqpprox_types.h>

#include <amqpprox_buffer.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_fieldvalue.h>

#include <boost/endian/arithmetic.hpp>

#include <gtest/gtest.h>

using Bloomberg::amqpprox::Buffer;
using Bloomberg::amqpprox::FieldTable;
using Bloomberg::amqpprox::FieldValue;
using Bloomberg::amqpprox::Types;

// this is a real response from a RabbitMQ server
int8_t serverProps[] = {
    12,  99,  97,  112, 97,  98,  105, 108, 105, 116, 105, 101, 115, 70,  0,
    0,   0,   -57, 18,  112, 117, 98,  108, 105, 115, 104, 101, 114, 95,  99,
    111, 110, 102, 105, 114, 109, 115, 116, 1,   26,  101, 120, 99,  104, 97,
    110, 103, 101, 95,  101, 120, 99,  104, 97,  110, 103, 101, 95,  98,  105,
    110, 100, 105, 110, 103, 115, 116, 1,   10,  98,  97,  115, 105, 99,  46,
    110, 97,  99,  107, 116, 1,   22,  99,  111, 110, 115, 117, 109, 101, 114,
    95,  99,  97,  110, 99,  101, 108, 95,  110, 111, 116, 105, 102, 121, 116,
    1,   18,  99,  111, 110, 110, 101, 99,  116, 105, 111, 110, 46,  98,  108,
    111, 99,  107, 101, 100, 116, 1,   19,  99,  111, 110, 115, 117, 109, 101,
    114, 95,  112, 114, 105, 111, 114, 105, 116, 105, 101, 115, 116, 1,   28,
    97,  117, 116, 104, 101, 110, 116, 105, 99,  97,  116, 105, 111, 110, 95,
    102, 97,  105, 108, 117, 114, 101, 95,  99,  108, 111, 115, 101, 116, 1,
    16,  112, 101, 114, 95,  99,  111, 110, 115, 117, 109, 101, 114, 95,  113,
    111, 115, 116, 1,   15,  100, 105, 114, 101, 99,  116, 95,  114, 101, 112,
    108, 121, 95,  116, 111, 116, 1,   12,  99,  108, 117, 115, 116, 101, 114,
    95,  110, 97,  109, 101, 83,  0,   0,   0,   14,  114, 97,  98,  98,  105,
    116, 64,  114, 97,  98,  98,  105, 116, 49,  9,   99,  111, 112, 121, 114,
    105, 103, 104, 116, 83,  0,   0,   0,   46,  67,  111, 112, 121, 114, 105,
    103, 104, 116, 32,  40,  67,  41,  32,  50,  48,  48,  55,  45,  50,  48,
    49,  54,  32,  80,  105, 118, 111, 116, 97,  108, 32,  83,  111, 102, 116,
    119, 97,  114, 101, 44,  32,  73,  110, 99,  46,  11,  105, 110, 102, 111,
    114, 109, 97,  116, 105, 111, 110, 83,  0,   0,   0,   53,  76,  105, 99,
    101, 110, 115, 101, 100, 32,  117, 110, 100, 101, 114, 32,  116, 104, 101,
    32,  77,  80,  76,  46,  32,  32,  83,  101, 101, 32,  104, 116, 116, 112,
    58,  47,  47,  119, 119, 119, 46,  114, 97,  98,  98,  105, 116, 109, 113,
    46,  99,  111, 109, 47,  8,   112, 108, 97,  116, 102, 111, 114, 109, 83,
    0,   0,   0,   10,  69,  114, 108, 97,  110, 103, 47,  79,  84,  80,  7,
    112, 114, 111, 100, 117, 99,  116, 83,  0,   0,   0,   8,   82,  97,  98,
    98,  105, 116, 77,  81,  7,   118, 101, 114, 115, 105, 111, 110, 83,  0,
    0,   0,   5,   51,  46,  54,  46,  50};

// this is a real value sent from the pika library
int8_t clientProps[] = {
    8,   112, 108, 97,  116, 102, 111, 114, 109, 83,  0,   0,   0,   13,  80,
    121, 116, 104, 111, 110, 32,  50,  46,  55,  46,  49,  49,  7,   112, 114,
    111, 100, 117, 99,  116, 83,  0,   0,   0,   26,  80,  105, 107, 97,  32,
    80,  121, 116, 104, 111, 110, 32,  67,  108, 105, 101, 110, 116, 32,  76,
    105, 98,  114, 97,  114, 121, 7,   118, 101, 114, 115, 105, 111, 110, 83,
    0,   0,   0,   6,   48,  46,  49,  48,  46,  48,  12,  99,  97,  112, 97,
    98,  105, 108, 105, 116, 105, 101, 115, 70,  0,   0,   0,   111, 18,  99,
    111, 110, 110, 101, 99,  116, 105, 111, 110, 46,  98,  108, 111, 99,  107,
    101, 100, 116, 1,   28,  97,  117, 116, 104, 101, 110, 116, 105, 99,  97,
    116, 105, 111, 110, 95,  102, 97,  105, 108, 117, 114, 101, 95,  99,  108,
    111, 115, 101, 116, 1,   22,  99,  111, 110, 115, 117, 109, 101, 114, 95,
    99,  97,  110, 99,  101, 108, 95,  110, 111, 116, 105, 102, 121, 116, 1,
    18,  112, 117, 98,  108, 105, 115, 104, 101, 114, 95,  99,  111, 110, 102,
    105, 114, 109, 115, 116, 1,   10,  98,  97,  115, 105, 99,  46,  110, 97,
    99,  107, 116, 1,   11,  105, 110, 102, 111, 114, 109, 97,  116, 105, 111,
    110, 83,  0,   0,   0,   24,  83,  101, 101, 32,  104, 116, 116, 112, 58,
    47,  47,  112, 105, 107, 97,  46,  114, 116, 102, 100, 46,  111, 114, 103};

TEST(TypesFieldTableDecode, ServerProps)
{
    std::vector<uint8_t>        buffer(sizeof(serverProps) + 4);
    boost::endian::big_uint32_t length = sizeof(serverProps);
    memcpy(buffer.data(), &length, sizeof(length));
    memcpy(buffer.data() + sizeof(length), serverProps, length);

    Buffer     data(buffer.data(), length + sizeof(length));
    FieldTable ft;
    bool       decodable = Types::decodeFieldTable(&ft, data);
    EXPECT_EQ(data.available(), 0);
    EXPECT_TRUE(decodable);

    std::vector<uint8_t> encodeBuffer(sizeof(serverProps) + 4);
    Buffer               encodeBuf(encodeBuffer.data(), encodeBuffer.size());
    bool                 encoded = Types::encodeFieldTable(encodeBuf, ft);
    EXPECT_TRUE(encoded);
}

TEST(TypesFieldTableDecode, ClientProps)
{
    std::vector<uint8_t>        buffer(sizeof(clientProps) + 4);
    boost::endian::big_uint32_t length = sizeof(clientProps);
    memcpy(buffer.data(), &length, sizeof(length));
    memcpy(buffer.data() + sizeof(length), clientProps, length);

    Buffer     data(buffer.data(), length + sizeof(length));
    FieldTable ft;
    bool       decodable = Types::decodeFieldTable(&ft, data);
    EXPECT_EQ(data.available(), 0);
    EXPECT_TRUE(decodable);

    std::vector<uint8_t> encodeBuffer(sizeof(clientProps) + 4);
    Buffer               encodeBuf(encodeBuffer.data(), encodeBuffer.size());
    bool                 encoded = Types::encodeFieldTable(encodeBuf, ft);
    EXPECT_TRUE(encoded);
}

// GENERAL CORRECTNESS TESTS
TEST(TypesEncoding, Breathing)
{
    EXPECT_TRUE(true);
}

TEST(TypesEncoding, ShouldRoundTripShortStringCorrectly)
{
    // GIVEN
    std::string shortString("ThisIsAShortString");

    std::vector<uint8_t> backingStore;
    backingStore.resize(256);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeShortString(buffer, shortString);

    buffer.seek(0);
    std::string resultString;
    bool        result = Types::decodeShortString(&resultString, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(shortString, resultString);
}

TEST(TypesEncoding, ShouldRejectShortEncodingStringTooLong)
{
    // GIVEN
    std::string longString(
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!");

    std::vector<uint8_t> backingStore;
    backingStore.resize(256);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    bool result = Types::encodeShortString(buffer, longString);

    // THEN
    EXPECT_FALSE(result);
    EXPECT_EQ(0, buffer.offset());
}

TEST(TypesEncoding, ShouldRoundTripLongStringCorrectly)
{
    // GIVEN
    std::string longString(
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!"
        "ThisIsALongerString!ThisIsALongerString!ThisIsALongerString!");

    std::vector<uint8_t> backingStore;
    backingStore.resize(1024);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeLongString(buffer, longString);

    buffer.seek(0);
    std::string resultString;
    bool        result = Types::decodeLongString(&resultString, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(longString, resultString);
}

TEST(TypesEncoding, ShouldRoundTripByteVectorCorrectly)
{
    // GIVEN
    std::vector<uint8_t> byteVector{10, 20, 30, 40, 50, 60, 70, 80};

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeByteVector(buffer, byteVector);

    buffer.seek(0);
    std::vector<uint8_t> resultVector;
    bool                 result =
        Types::decodeByteVector(&resultVector, buffer, byteVector.size());

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(byteVector, resultVector);
}

TEST(TypesEncoding, ShouldRoundTripFieldValueBoolCorrectly)
{
    // GIVEN
    FieldValue fieldValue('t', true);

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldValue(buffer, fieldValue);

    buffer.seek(0);
    FieldValue resultFieldValue('V', static_cast<bool>(0));
    bool       result = Types::decodeFieldValue(&resultFieldValue, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldValue, resultFieldValue);
}

TEST(TypesEncoding, ShouldRoundTripFieldValueFloatCorrectly)
{
    // GIVEN
    float                floatValue = 5.32745;
    std::vector<uint8_t> encodedFloat(4);
    memcpy(encodedFloat.data(), static_cast<void *>(&floatValue), 4);

    FieldValue fieldValue('f', encodedFloat);

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldValue(buffer, fieldValue);

    buffer.seek(0);
    FieldValue resultFieldValue('V', static_cast<bool>(0));
    bool       result = Types::decodeFieldValue(&resultFieldValue, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldValue, resultFieldValue);

    std::vector<uint8_t> decodedFloatVector(
        resultFieldValue.value<std::vector<uint8_t>>());
    float decodedFloatValue;
    memcpy(
        static_cast<void *>(&decodedFloatValue), decodedFloatVector.data(), 4);

    EXPECT_EQ(floatValue, decodedFloatValue);
}

TEST(TypesEncoding, ShouldRoundTripFieldValueDoubleCorrectly)
{
    // GIVEN
    double               doubleValue = 5.32745193786354297;
    std::vector<uint8_t> encodedDouble(8);
    memcpy(encodedDouble.data(), static_cast<void *>(&doubleValue), 8);

    FieldValue fieldValue('d', encodedDouble);

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldValue(buffer, fieldValue);

    buffer.seek(0);
    FieldValue resultFieldValue('V', static_cast<bool>(0));
    bool       result = Types::decodeFieldValue(&resultFieldValue, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldValue, resultFieldValue);

    std::vector<uint8_t> decodedDoubleVector(
        resultFieldValue.value<std::vector<uint8_t>>());
    double decodedDoubleValue;
    memcpy(static_cast<void *>(&decodedDoubleValue),
           decodedDoubleVector.data(),
           8);

    EXPECT_EQ(doubleValue, decodedDoubleValue);
}

TEST(TypesEncoding, ShouldRoundTripFieldValueDecimalCorrectly)
{
    // GIVEN
    uint8_t  precision = 2;
    uint32_t value     = 1011;

    std::vector<uint8_t> encodedDecimal(5);
    memcpy(&(encodedDecimal[0]), static_cast<void *>(&precision), 1);
    memcpy(&(encodedDecimal[1]), static_cast<void *>(&value), 4);

    FieldValue fieldValue('D', encodedDecimal);

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldValue(buffer, fieldValue);

    buffer.seek(0);
    FieldValue resultFieldValue('V', static_cast<bool>(0));
    bool       result = Types::decodeFieldValue(&resultFieldValue, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldValue, resultFieldValue);

    std::vector<uint8_t> decodedDecimal(
        resultFieldValue.value<std::vector<uint8_t>>());

    uint8_t  decodedPrecision;
    uint32_t decodedValue;
    memcpy(&decodedPrecision, static_cast<void *>(&(decodedDecimal[0])), 1);
    memcpy(&decodedValue, static_cast<void *>(&(decodedDecimal[1])), 4);

    EXPECT_EQ(precision, decodedPrecision);
    EXPECT_EQ(value, decodedValue);
}

TEST(TypesEncoding, ShouldRoundTripFieldArrayCorrectly)
{
    // GIVEN
    FieldValue fieldValue1('t', true);
    FieldValue fieldValue2('t', false);
    FieldValue fieldValue3('t', true);

    std::vector<FieldValue> fieldArray{fieldValue1, fieldValue2, fieldValue3};

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldArray(buffer, fieldArray);

    buffer.seek(0);
    std::vector<FieldValue> resultFieldArray;
    bool result = Types::decodeFieldArray(&resultFieldArray, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldArray, resultFieldArray);
}

TEST(TypesEncoding, ShouldRoundTripFieldValueArrayCorrectly)
{
    // GIVEN
    FieldValue fieldValue1('t', true);
    FieldValue fieldValue2('t', false);
    FieldValue fieldValue3('t', true);

    std::vector<FieldValue> fieldArray{fieldValue1, fieldValue2, fieldValue3};
    FieldValue              fieldValue('A', fieldArray);

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldValue(buffer, fieldValue);

    buffer.seek(0);
    FieldValue resultFieldValue('V', static_cast<bool>(0));
    bool       result = Types::decodeFieldValue(&resultFieldValue, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldValue, resultFieldValue);
}

TEST(TypesEncoding, ShouldRoundTripFieldArrayStringsAndBytesCorrectly)
{
    // GIVEN
    float                floatValue = 5.32745;
    std::vector<uint8_t> encodedFloat(4);
    memcpy(encodedFloat.data(), static_cast<void *>(&floatValue), 4);

    FieldValue fieldValue1('s', std::string("ThisIsAShortString!"));
    FieldValue fieldValue2('f', encodedFloat);
    FieldValue fieldValue3('t', true);

    std::vector<FieldValue> fieldArray{fieldValue1, fieldValue2, fieldValue3};

    std::vector<uint8_t> backingStore;
    backingStore.resize(128);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    Types::encodeFieldArray(buffer, fieldArray);

    buffer.seek(0);
    std::vector<FieldValue> resultFieldArray;
    bool result = Types::decodeFieldArray(&resultFieldArray, buffer);

    // THEN
    EXPECT_TRUE(result);
    EXPECT_EQ(fieldArray, resultFieldArray);
}
