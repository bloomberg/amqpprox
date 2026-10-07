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
#include <amqpprox_types.h>

#include <amqpprox_buffer.h>
#include <amqpprox_constants.h>
#include <amqpprox_fieldtable.h>
#include <amqpprox_fieldvalue.h>
#include <amqpprox_frame.h>

#include <boost/endian/arithmetic.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <ostream>
#include <stdexcept>

using Bloomberg::amqpprox::Buffer;
using Bloomberg::amqpprox::Constants;
using Bloomberg::amqpprox::FieldTable;
using Bloomberg::amqpprox::FieldValue;
using Bloomberg::amqpprox::Frame;
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

    FieldValue fieldValue1('S', std::string("ThisIsAString!"));
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

TEST(TypesAMQPExceptionals, ShouldPreserveCompatibilityTypes)
{
    // GIVEN
    FieldValue fieldValue1('s', int64_t(1337));
    FieldValue fieldValue2('l', int64_t(1333333333333333337));
    FieldValue fieldValue3('L', int64_t(7333333333333333331));
    FieldValue fieldValue4('S', std::string("ThisIsAString"));
    FieldValue fieldValue5('x', std::vector<uint8_t>({1, 3, 3, 7}));

    std::vector<FieldValue> fieldArray{
        fieldValue1, fieldValue2, fieldValue3, fieldValue4};

    std::vector<uint8_t> backingStore;
    backingStore.resize(512);
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

TEST(TypesAMQPExceptionals, ShouldConvertUTosForShortInt)
{
    FieldValue expected('s', int64_t(1337));

    std::vector<uint8_t> backingStore;
    backingStore.resize(32);
    Buffer buffer(backingStore.data(), backingStore.capacity());

    // WHEN
    bool result     = Types::encodeFieldValue(buffer, expected);
    backingStore[0] = 'U';  // manually override to amqp 'U' field

    EXPECT_TRUE(result);

    buffer.seek(0);

    FieldValue decodedField('V', false);
    result = Types::decodeFieldValue(&decodedField, buffer);

    EXPECT_TRUE(result);
    EXPECT_EQ(decodedField, expected);
}

// TRUNCATED SCALAR FIELD VALUES
//
// A peer can declare a scalar type and then not supply its octets; decoding
// must reject that rather than read past the end.

// Every .t.cpp links into the single amqpprox_tests binary, so file-local
// helpers go in an anonymous namespace to keep them from colliding.
namespace {

struct ScalarFieldType {
    char        d_type;
    std::size_t d_valueOctets;
};

std::ostream &operator<<(std::ostream &os, const ScalarFieldType &scalarType)
{
    return os << "type '" << scalarType.d_type << "' of "
              << scalarType.d_valueOctets << " octets";
}

}

class TypesTruncatedScalar : public ::testing::TestWithParam<ScalarFieldType> {
};

INSTANTIATE_TEST_SUITE_P(
    AllScalarFieldTypes,
    TypesTruncatedScalar,
    ::testing::Values(ScalarFieldType{'t', 1},    // boolean
                      ScalarFieldType{'b', 1},    // short-short-int
                      ScalarFieldType{'B', 1},    // short-short-uint
                      ScalarFieldType{'s', 2},    // short-int
                      ScalarFieldType{'u', 2},    // short-uint
                      ScalarFieldType{'U', 2},    // decoded as short-int
                      ScalarFieldType{'I', 4},    // long-int
                      ScalarFieldType{'i', 4},    // long-uint
                      ScalarFieldType{'l', 8},    // long-long-int
                      ScalarFieldType{'L', 8},    // compat long-long-int
                      ScalarFieldType{'T', 8},    // timestamp
                      ScalarFieldType{'f', 4},    // float
                      ScalarFieldType{'d', 8},    // double
                      ScalarFieldType{'D', 5}));  // decimal-value

TEST_P(TypesTruncatedScalar, ShouldRejectValueTruncatedAtAnyLength)
{
    const ScalarFieldType scalarType = GetParam();

    for (std::size_t present = 0; present < scalarType.d_valueOctets;
         ++present) {
        std::vector<uint8_t> backingStore(1 + present, 0);
        backingStore[0] = static_cast<uint8_t>(scalarType.d_type);

        Buffer buffer(backingStore.data(), backingStore.size());

        FieldValue decodedField('V', false);
        EXPECT_FALSE(Types::decodeFieldValue(&decodedField, buffer))
            << "accepted " << scalarType << " with only " << present
            << " value octets present";
    }
}

// Guards the test above from passing vacuously on an unrecognised type byte.
TEST_P(TypesTruncatedScalar, ShouldAcceptValueWithAllOctetsPresent)
{
    const ScalarFieldType scalarType = GetParam();

    std::vector<uint8_t> backingStore(1 + scalarType.d_valueOctets, 0);
    backingStore[0] = static_cast<uint8_t>(scalarType.d_type);

    Buffer buffer(backingStore.data(), backingStore.size());

    FieldValue decodedField('V', false);
    EXPECT_TRUE(Types::decodeFieldValue(&decodedField, buffer))
        << "rejected a complete " << scalarType;
    EXPECT_EQ(buffer.available(), 0);
}

// The reported shape: the table length matches the octets supplied, so the
// table is well formed and only the value inside it is short.
TEST_P(TypesTruncatedScalar, ShouldRejectValueTruncatedInsideFieldTable)
{
    const ScalarFieldType scalarType = GetParam();

    for (std::size_t present = 0; present < scalarType.d_valueOctets;
         ++present) {
        // Empty field name, type byte, then a value cut short.
        boost::endian::big_uint32_t tableLength = 2 + present;

        std::vector<uint8_t> backingStore(sizeof(tableLength));
        memcpy(backingStore.data(), &tableLength, sizeof(tableLength));
        backingStore.push_back(0x00);
        backingStore.push_back(static_cast<uint8_t>(scalarType.d_type));
        backingStore.resize(backingStore.size() + present, 0);

        Buffer buffer(backingStore.data(), backingStore.size());

        FieldTable table;
        EXPECT_FALSE(Types::decodeFieldTable(&table, buffer))
            << "accepted " << scalarType << " with only " << present
            << " value octets present inside a field table";
    }
}

// Field arrays reach decodeFieldValue by a different route to field tables, so
// the table case above does not cover them.
TEST_P(TypesTruncatedScalar, ShouldRejectValueTruncatedInsideFieldArray)
{
    const ScalarFieldType scalarType = GetParam();

    for (std::size_t present = 0; present < scalarType.d_valueOctets;
         ++present) {
        // Type byte then a value cut short - arrays carry no field names.
        boost::endian::big_uint32_t arrayLength = 1 + present;

        std::vector<uint8_t> backingStore(sizeof(arrayLength));
        memcpy(backingStore.data(), &arrayLength, sizeof(arrayLength));
        backingStore.push_back(static_cast<uint8_t>(scalarType.d_type));
        backingStore.resize(backingStore.size() + present, 0);

        Buffer buffer(backingStore.data(), backingStore.size());

        std::vector<FieldValue> values;
        EXPECT_FALSE(Types::decodeFieldArray(&values, buffer))
            << "accepted " << scalarType << " with only " << present
            << " value octets present inside a field array";
    }
}

// MISSING TYPE OCTET
//
// This is the case the bounds check on the type octet itself exists for, and
// the one that was actually reachable in production: decodeFieldTable's loop
// runs `while (tBuffer.available() > 0)`, decodeShortString can consume the
// entire remainder, and decodeFieldValue is then entered unconditionally with
// nothing left. The parameterised truncation tests above all supply a type
// octet, so none of them cover it.

TEST(TypesTruncated, ShouldRejectFieldTableWhoseLastFieldHasNoTypeOctet)
{
    // A table whose payload is a one-character field name and nothing else.
    boost::endian::big_uint32_t tableLength = 2;

    std::vector<uint8_t> backingStore(sizeof(tableLength));
    memcpy(backingStore.data(), &tableLength, sizeof(tableLength));
    backingStore.push_back(0x01);  // field name length
    backingStore.push_back('a');   // field name, consuming the remainder

    Buffer     buffer(backingStore.data(), backingStore.size());
    FieldTable table;

    EXPECT_FALSE(Types::decodeFieldTable(&table, buffer));
}

TEST(TypesTruncated, ShouldRejectFieldValueWithNoTypeOctet)
{
    std::vector<uint8_t> backingStore;

    Buffer     buffer(backingStore.data(), backingStore.size());
    FieldValue decodedField('V', false);

    EXPECT_FALSE(Types::decodeFieldValue(&decodedField, buffer));
}

// TRUNCATED LENGTH PREFIXES
//
// The variable length types read a length prefix before their payload. Those
// reads were already guarded, so converting them to tryCopy was a refactor -
// but a refactor of a bounds check with no test is what regresses later.

TEST(TypesTruncated, ShouldRejectVariableLengthValueWithTruncatedLengthPrefix)
{
    const char types[] = {'S', 'x', 'A', 'F'};

    for (char type : types) {
        for (std::size_t present = 0; present < sizeof(uint32_t); ++present) {
            std::vector<uint8_t> backingStore;
            backingStore.push_back(static_cast<uint8_t>(type));
            backingStore.resize(backingStore.size() + present, 0);

            Buffer     buffer(backingStore.data(), backingStore.size());
            FieldValue decodedField('V', false);

            EXPECT_FALSE(Types::decodeFieldValue(&decodedField, buffer))
                << "accepted type '" << type << "' with only " << present
                << " of 4 length-prefix octets";
        }
    }
}

// ENCODING INTO AN UNDERSIZED BUFFER
//
// encodeFieldTable and encodeFieldArray reserve the four-octet length prefix
// before writing anything. That reservation used to be an unchecked `skip`,
// which pushed the offset past the end; because `available()` is unsigned it
// then underflowed, `writeIn`'s own bounds check passed, and memcpy wrote off
// the end of the output buffer.
//
// Both functions are documented to return false on failure, so these tests
// call them without a try/catch on purpose: an exception escaping here fails
// the test, which is the point. The buffers are heap-sized exactly so a
// sanitizer build also catches any write past the end.
//
// The range runs past the four-octet prefix so that both regimes are covered:
// too small for the prefix at all, and prefix fits but the field does not.

TEST(TypesEncodingBounds, ShouldNotWritePastEndOfUndersizedFieldTableBuffer)
{
    FieldTable table;
    table.pushField("k", FieldValue('t', true));

    // 4 octet length prefix, 1 name length, 1 name octet, 1 type octet,
    // 1 boolean octet.
    const std::size_t exactFit = 8;

    for (std::size_t outputOctets = 0; outputOctets < exactFit;
         ++outputOctets) {
        // Heap allocated rather than a vector so that a zero-octet buffer is
        // still a real allocation a sanitizer can put a redzone around,
        // instead of a null pointer.
        std::unique_ptr<uint8_t[]> backingStore(new uint8_t[outputOctets]);
        Buffer                     buffer(backingStore.get(), outputOctets);

        EXPECT_FALSE(Types::encodeFieldTable(buffer, table))
            << "claimed to encode a field table into " << outputOctets
            << " octets";
    }

    // Positive control - without it the test would still pass if the new
    // bounds check were so strict that nothing encoded at all.
    std::unique_ptr<uint8_t[]> backingStore(new uint8_t[exactFit]);
    Buffer                     buffer(backingStore.get(), exactFit);
    EXPECT_TRUE(Types::encodeFieldTable(buffer, table))
        << "rejected a field table that fits in exactly " << exactFit
        << " octets";
}

TEST(TypesEncodingBounds, ShouldNotWritePastEndOfUndersizedFieldArrayBuffer)
{
    std::vector<FieldValue> values;
    values.push_back(FieldValue('t', true));

    // 4 octet length prefix, 1 type octet, 1 boolean octet. Arrays carry no
    // field names.
    const std::size_t exactFit = 6;

    for (std::size_t outputOctets = 0; outputOctets < exactFit;
         ++outputOctets) {
        std::unique_ptr<uint8_t[]> backingStore(new uint8_t[outputOctets]);
        Buffer                     buffer(backingStore.get(), outputOctets);

        EXPECT_FALSE(Types::encodeFieldArray(buffer, values))
            << "claimed to encode a field array into " << outputOctets
            << " octets";
    }

    std::unique_ptr<uint8_t[]> backingStore(new uint8_t[exactFit]);
    Buffer                     buffer(backingStore.get(), exactFit);
    EXPECT_TRUE(Types::encodeFieldArray(buffer, values))
        << "rejected a field array that fits in exactly " << exactFit
        << " octets";
}

// The nested case. encodeFieldValue's 'F' and 'A' branches advance the output
// with `writeBuffer.skip(spaceLeft.offset())`, which is the one remaining
// unchecked skip in the encode path. It is in range by construction rather
// than by a check, so pin it with a test instead of leaving it to argument.
TEST(TypesEncodingBounds, ShouldNotWritePastEndWhenEncodingANestedFieldTable)
{
    FieldTable inner;
    FieldTable outer;
    outer.pushField("", FieldValue('F', std::make_shared<FieldTable>(inner)));

    // 4 outer prefix, 1 name length, 1 type octet, 4 inner prefix.
    const std::size_t exactFit = 10;

    for (std::size_t outputOctets = 0; outputOctets < exactFit;
         ++outputOctets) {
        std::unique_ptr<uint8_t[]> backingStore(new uint8_t[outputOctets]);
        Buffer                     buffer(backingStore.get(), outputOctets);

        EXPECT_FALSE(Types::encodeFieldTable(buffer, outer))
            << "claimed to encode a nested field table into " << outputOctets
            << " octets";
    }

    std::unique_ptr<uint8_t[]> backingStore(new uint8_t[exactFit]);
    Buffer                     buffer(backingStore.get(), exactFit);
    EXPECT_TRUE(Types::encodeFieldTable(buffer, outer))
        << "rejected a nested field table that fits in exactly " << exactFit
        << " octets";
}

// NESTING DEPTH
//
// decodeFieldTable -> decodeFieldValue -> decodeFieldTable recurses once per
// nesting level at a cost of only six octets of input per level, so a single
// maximum-sized frame can demand tens of thousands of stack frames. Stack
// exhaustion cannot be caught, so it cannot be contained by handleData the way
// a truncated value can.

namespace {

// Append a four-octet big-endian length to `out`.
void appendLength(std::vector<uint8_t> *out, std::size_t value)
{
    boost::endian::big_uint32_t length = value;
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&length);
    out->insert(out->end(), bytes, bytes + sizeof(length));
}

// [length][empty field name][type 'F'] repeated, innermost an empty table.
// Each level costs six octets; the whole encoding is 6 * depth + 4.
std::vector<uint8_t> makeNestedFieldTable(std::size_t depth)
{
    std::vector<uint8_t> encoded;
    encoded.reserve(6 * depth + sizeof(boost::endian::big_uint32_t));

    for (std::size_t level = depth; level > 0; --level) {
        appendLength(&encoded, 6 * level);
        encoded.push_back(0x00);  // zero-length field name
        encoded.push_back('F');   // value is a nested field table
    }

    appendLength(&encoded, 0);

    return encoded;
}

// [length][type 'A'] repeated, innermost an empty array. Arrays carry no field
// names, so each level costs five octets rather than six - a cheaper bomb than
// the field table equivalent.
std::vector<uint8_t> makeNestedFieldArray(std::size_t depth)
{
    std::vector<uint8_t> encoded;
    encoded.reserve(5 * depth + sizeof(boost::endian::big_uint32_t));

    for (std::size_t level = depth; level > 0; --level) {
        appendLength(&encoded, 5 * level);
        encoded.push_back('A');  // value is a nested field array
    }

    appendLength(&encoded, 0);

    return encoded;
}

// Mixed array/table nesting, built innermost outward, so the mixed case is
// covered rather than only a chain of one container type. Levels alternate
// except that the outermost is forced to a table so the test can drive
// decodeFieldTable; for some depths that leaves two adjacent table levels,
// which is immaterial because every container counts one level whatever its
// type.
std::vector<uint8_t> makeAlternatingNestedTable(std::size_t depth)
{
    std::vector<uint8_t> encoded;
    appendLength(&encoded, 0);  // innermost is an empty table
    char innerType = 'F';

    for (std::size_t level = 0; level < depth; ++level) {
        const bool wrapInTable = (level % 2 == 1) || (level + 1 == depth);

        std::vector<uint8_t> wrapped;
        appendLength(&wrapped, encoded.size() + (wrapInTable ? 2 : 1));
        if (wrapInTable) {
            wrapped.push_back(0x00);  // zero-length field name
        }
        wrapped.push_back(static_cast<uint8_t>(innerType));
        wrapped.insert(wrapped.end(), encoded.begin(), encoded.end());

        encoded.swap(wrapped);
        innerType = wrapInTable ? 'F' : 'A';
    }

    return encoded;
}

}

TEST(TypesNesting, ShouldSurviveMaximallyNestedFieldTableInOneFrame)
{
    // The deepest nesting a single maximum-sized frame can carry. Without a
    // depth limit this segfaults on an ordinary 8MB stack - verified without
    // sanitizers, so it is not an instrumentation artifact.
    const std::size_t maxFrameOctets = Frame::getMaxFrameSize();
    const std::size_t depth =
        (maxFrameOctets - sizeof(boost::endian::big_uint32_t)) / 6;

    // Frame::maxFrameSize is a mutable static. If another test in this binary
    // ever lowers it, `depth` collapses and this would assert against a table
    // that is legitimately shallow enough to accept.
    ASSERT_GT(depth, Constants::maxFieldTableNestingDepth());

    std::vector<uint8_t> encoded = makeNestedFieldTable(depth);
    ASSERT_LE(encoded.size(), maxFrameOctets);

    Buffer     buffer(encoded.data(), encoded.size());
    FieldTable table;

    EXPECT_FALSE(Types::decodeFieldTable(&table, buffer))
        << "accepted " << depth << " levels of nesting";
}

TEST(TypesNesting, ShouldAcceptNestingUpToTheDepthLimit)
{
    // Guards the limit from being tightened to the point where it would start
    // rejecting real client properties, which nest a capabilities table one
    // level inside the client-properties table.
    const std::size_t depth = Constants::maxFieldTableNestingDepth();

    std::vector<uint8_t> encoded = makeNestedFieldTable(depth);

    Buffer     buffer(encoded.data(), encoded.size());
    FieldTable table;

    EXPECT_TRUE(Types::decodeFieldTable(&table, buffer))
        << "rejected " << depth << " levels, which is within the limit";
}

TEST(TypesNesting, ShouldRejectNestingPastTheDepthLimit)
{
    const std::size_t depth = Constants::maxFieldTableNestingDepth() + 1;

    std::vector<uint8_t> encoded = makeNestedFieldTable(depth);

    Buffer     buffer(encoded.data(), encoded.size());
    FieldTable table;

    EXPECT_FALSE(Types::decodeFieldTable(&table, buffer))
        << "accepted " << depth << " levels, which is past the limit";
}

// Field arrays recurse through a different entry point to field tables, and at
// five octets per level are the cheaper of the two to abuse.
TEST(TypesNesting, ShouldBoundFieldArrayNestingToo)
{
    const std::size_t withinLimit = Constants::maxFieldTableNestingDepth();

    std::vector<uint8_t> withinEncoded = makeNestedFieldArray(withinLimit);
    Buffer withinBuffer(withinEncoded.data(), withinEncoded.size());
    std::vector<FieldValue> withinValues;
    EXPECT_TRUE(Types::decodeFieldArray(&withinValues, withinBuffer))
        << "rejected " << withinLimit << " levels of array nesting";

    const std::size_t pastLimit = withinLimit + 1;

    std::vector<uint8_t>    pastEncoded = makeNestedFieldArray(pastLimit);
    Buffer                  pastBuffer(pastEncoded.data(), pastEncoded.size());
    std::vector<FieldValue> pastValues;
    EXPECT_FALSE(Types::decodeFieldArray(&pastValues, pastBuffer))
        << "accepted " << pastLimit << " levels of array nesting";
}

// Alternating the two container types must not let the count slip: both
// recursive edges have to increment the same depth.
TEST(TypesNesting, ShouldBoundAlternatingArrayAndTableNesting)
{
    const std::size_t withinLimit = Constants::maxFieldTableNestingDepth();

    std::vector<uint8_t> withinEncoded =
        makeAlternatingNestedTable(withinLimit);
    Buffer     withinBuffer(withinEncoded.data(), withinEncoded.size());
    FieldTable withinTable;
    EXPECT_TRUE(Types::decodeFieldTable(&withinTable, withinBuffer))
        << "rejected " << withinLimit << " alternating levels";

    const std::size_t pastLimit = withinLimit + 1;

    std::vector<uint8_t> pastEncoded = makeAlternatingNestedTable(pastLimit);
    Buffer               pastBuffer(pastEncoded.data(), pastEncoded.size());
    FieldTable           pastTable;
    EXPECT_FALSE(Types::decodeFieldTable(&pastTable, pastBuffer))
        << "accepted " << pastLimit << " alternating levels";
}
