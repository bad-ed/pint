#include <gtest/gtest.h>
#include "pint/pint.hpp"

TEST(TestMakeTruncate, InputWithoutOverflow)
{
    constexpr auto result = pint::make_truncate<uint16_t, 5, 6, 5>(1, 20, 10);
    constexpr uint16_t expected_result = 1 | (20 << 5) | (10 << 11);

    ASSERT_EQ(expected_result, result);
}

TEST(TestMakeTruncate, InputWithOverflow)
{
    constexpr auto result = pint::make_truncate<uint16_t, 5, 6, 5>(33, 66, 234);
    constexpr uint16_t expected_result = (33 & 0x1F) | ((66 & 0x3F) << 5) | ((234 & 0x1F) << 11);

    ASSERT_EQ(expected_result, result);
}

TEST(TestAddWrap, NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(1, 20, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(3, 2, 1);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 5, 6, 5>(
        1 + 3, 20 + 2, 10 + 1);

    ASSERT_EQ(expected_sum, (pint::add_wrap<5,6,5>(a, b)));
}

TEST(TestAddWrap, WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(1, 60, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(31, 20, 27);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 5, 6, 5>(
        31 + 1, 60 + 20, 10 + 27);

    ASSERT_EQ(expected_sum, (pint::add_wrap<5,6,5>(a, b)));
}

TEST(TestAddWrap, WithOverflow2) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 3, 3>(3, 4, 5);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 3, 3>(5, 6, 7);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 3, 3, 3>(
        3 + 5, 4 + 6, 5 + 7);

    ASSERT_EQ(expected_sum, (pint::add_wrap<3,3,3>(a, b)));
}

TEST(TestAddWrap, WithOverflow_1BitPacks) {
    constexpr auto a = pint::make_truncate<uint16_t, 1, 1, 1>(1, 0, 1);
    constexpr auto b = pint::make_truncate<uint16_t, 1, 1, 1>(0, 0, 1);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 1, 1, 1>(1,0,0);

    ASSERT_EQ(expected_sum, (pint::add_wrap<1,1,1>(a, b)));
}

TEST(TestAddUnsignedSaturate, EqualLength_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 3, 3>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 3, 3>(2, 3, 4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 3, 3, 3>(
        1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, (pint::add_unsigned_saturate<3,3,3>(a, b)));
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 3, 3>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 3, 3>(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = pint::make_truncate<uint16_t, 3, 3, 3>(7,6,7);

    ASSERT_EQ(expected_sum, (pint::add_unsigned_saturate<3,3,3>(a, b)));
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow_1BitPacks) {
    constexpr auto a = pint::make_truncate<uint16_t, 1, 1, 1>(1, 0, 1);
    constexpr auto b = pint::make_truncate<uint16_t, 1, 1, 1>(0, 0, 1);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 1, 1, 1>(1,0,1);

    ASSERT_EQ(expected_sum, (pint::add_unsigned_saturate<1,1,1>(a, b)));
}

TEST(TestAddUnsignedSaturate, VarLength_WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 4, 3>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 4, 3>(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = pint::make_truncate<uint16_t, 3, 4, 3>(7,6,7);

    ASSERT_EQ(expected_sum, (pint::add_unsigned_saturate<3,4,3>(a, b)));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, EqualLength_Positive_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 4, 4>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 4, 4>(2, 3, 4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 4, 4>(
        1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,4,4>(a, b)));
}

TEST(TestAddSignedSaturate, EqualLength_Negative_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 4, 4>(-1, -2, -3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 4, 4>(-2, -3, -4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 4, 4>(
        -1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,4,4>(a, b)));
}

TEST(TestAddSignedSaturate, EqualLength_PositiveNegative_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 4, 4>(1, -2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 4, 4>(-2, 3, -4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 4, 4>(
        1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,4,4>(a, b)));
}

TEST(TestAddSignedSaturate, EqualLength_Positive_Overflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 4, 4>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 4, 4>(7, 4, 6);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 4, 4>(7, 6, 7);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,4,4>(a, b)));
}

TEST(TestAddSignedSaturate, EqualLength_Negative_Overflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 4, 4>(-1, -2, -3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 4, 4>(-8, -4, -6);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 4, 4>(-8, -6, -8);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,4,4>(a, b)));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, VarLength_Positive_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 5, 4>(1, 2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 5, 4>(2, 3, 4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 5, 4>(
        1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,5,4>(a, b)));
}

TEST(TestAddSignedSaturate, VarLength_Negative_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 5, 4>(-1, -2, -3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 5, 4>(-2, -3, -4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 5, 4>(
        -1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,5,4>(a, b)));
}

TEST(TestAddSignedSaturate, VarLength_PositiveNegative_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 5, 4>(1, -2, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 5, 4>(-2, 3, -4);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 5, 4>(
        1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,5,4>(a, b)));
}

TEST(TestAddSignedSaturate, VarLength_Positive_Overflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 5, 4>(1, 10, 3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 5, 4>(7, 14, 6);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 5, 4>(7, 15, 7);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,5,4>(a, b)));
}

TEST(TestAddSignedSaturate, VarLength_Negative_Overflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 5, 4>(-1, -12, -3);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 5, 4>(-8, -14, -6);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 4, 5, 4>(-8, -16, -8);

    ASSERT_EQ(expected_sum, (pint::add_signed_saturate<4,5,4>(a, b)));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubWrap, NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(4, 20, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(3, 2, 1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 5, 6, 5>(
        4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, (pint::sub_wrap<5,6,5>(a, b)));
}

TEST(TestSubWrap, NoOverflow2) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 3, 3>(7, 6, 5);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 3, 3>(1, 2, 3);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 3, 3, 3>(
        7 - 1, 6 - 2, 5 - 3);

    ASSERT_EQ(expected_diff, (pint::sub_wrap<3,3,3>(a, b)));
}

TEST(TestSubWrap, NoOverflow_1BitPacks) {
    constexpr auto a = pint::make_truncate<uint16_t, 1, 1, 1>(1, 1, 0);
    constexpr auto b = pint::make_truncate<uint16_t, 1, 1, 1>(1, 0, 0);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 1, 1, 1>(
        1 - 1, 1 - 0, 0 - 0);

    ASSERT_EQ(expected_diff, (pint::sub_wrap<1,1,1>(a, b)));
}

TEST(TestSubWrap, WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 3, 3, 3>(1, 4, 2);
    constexpr auto b = pint::make_truncate<uint16_t, 3, 3, 3>(7, 2, 6);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 3, 3, 3>(
        1 - 7, 4 - 2, 2 - 6);

    ASSERT_EQ(expected_diff, (pint::sub_wrap<3,3,3>(a, b)));
}

TEST(TestSubWrap, WithOverflow_1BitPacks) {
    constexpr auto a = pint::make_truncate<uint16_t, 1, 1, 1>(1, 0, 0);
    constexpr auto b = pint::make_truncate<uint16_t, 1, 1, 1>(1, 1, 0);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 1, 1, 1>(
        1 - 1, 0 - 1, 0 - 0);

    ASSERT_EQ(expected_diff, (pint::sub_wrap<1,1,1>(a, b)));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubUnsignedSaturate, NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(4, 20, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(3, 2, 1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 5, 6, 5>(
        4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, (pint::sub_unsigned_saturate<5,6,5>(a, b)));
}

TEST(TestSubUnsignedSaturate, WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(4, 2, 1);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(3, 20, 10);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 5, 6, 5>(1, 0, 0);

    ASSERT_EQ(expected_diff, (pint::sub_unsigned_saturate<5,6,5>(a, b)));
}

TEST(TestSubUnsignedSaturate, WithOverflow_1BitPacks) {
    constexpr auto a = pint::make_truncate<uint16_t, 1, 1, 1>(1, 0, 0);
    constexpr auto b = pint::make_truncate<uint16_t, 1, 1, 1>(1, 1, 0);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 1, 1, 1>(
        0, 0, 0);

    ASSERT_EQ(expected_diff, (pint::sub_unsigned_saturate<1,1,1>(a, b)));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubSignedSaturate, Positive_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(4, 20, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(3, 2, 1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 5, 6, 5>(
        4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, (pint::sub_signed_saturate<5,6,5>(a, b)));
}

TEST(TestSubSignedSaturate, Negative_NoOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(-4, -20, -10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(-3, -2, -1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 5, 6, 5>(
        -4 - -3, -20 - -2, -10 - -1);

    ASSERT_EQ(expected_diff, (pint::sub_signed_saturate<5,6,5>(a, b)));
}

TEST(TestSubSignedSaturate, PositiveNegativeOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 6, 4>(4, 0, 7);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 6, 4>(-6, -32, 1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 4, 6, 4>(
        7, 31, 6);

    ASSERT_EQ(expected_diff, (pint::sub_signed_saturate<4,6,4>(a, b)));
}

TEST(TestSubSignedSaturate, NegativePositiveOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 4, 6, 4>(-4, -2, -6);
    constexpr auto b = pint::make_truncate<uint16_t, 4, 6, 4>(6, 30, 1);

    constexpr auto expected_diff = pint::make_truncate<uint16_t, 4, 6, 4>(
        -8, -32, -7);

    ASSERT_EQ(expected_diff, (pint::sub_signed_saturate<4,6,4>(a, b)));
}
