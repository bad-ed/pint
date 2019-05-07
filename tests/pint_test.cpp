#include <gtest/gtest.h>
#include "pint/pint.hpp"

TEST(TestMakeTruncate, InputWithoutOverflow)
{
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto result = PackedInt(1, 20, 10);
    constexpr uint16_t expected_result = 1 | (20 << 5) | (10 << 11);

    ASSERT_EQ(expected_result, result.value());
}

TEST(TestMakeTruncate, InputWithOverflow)
{
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto result = PackedInt(33, 66, 234);
    constexpr uint16_t expected_result = (33 & 0x1F) | ((66 & 0x3F) << 5) | ((234 & 0x1F) << 11);

    ASSERT_EQ(expected_result, result.value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestGet, GetUnsigned) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto result = PackedInt(1, 20, 10);

    ASSERT_EQ(1, pint::get<0>(result));
    ASSERT_EQ(20, pint::get<1>(result));
    ASSERT_EQ(10, pint::get<2>(result));
}

TEST(TestGet, GetSigned) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto result = PackedInt(1, -3, -10);

    ASSERT_EQ(1, pint::get_signed<0>(result));
    ASSERT_EQ(-3, pint::get_signed<1>(result));
    ASSERT_EQ(-10, pint::get_signed<2>(result));

    ASSERT_NE(-3, pint::get<1>(result));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSlice, Slice) {
    using PackedInt = pint::packed_int<uint16_t, 1, 2, 3, 4, 5>;
    using SlicedInt = pint::packed_int<uint16_t, 3, 4>;

    constexpr auto value = PackedInt(1, 2, 3, 4, 5);
    constexpr auto sliced = pint::slice<2, 4>(value);

    static_assert(std::is_same<std::decay<decltype(sliced)>::type, SlicedInt>::value,
        "Wrong type of sliced value");

    ASSERT_EQ(sliced.value(), SlicedInt(3,4).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddWrap, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(1, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_sum = PackedInt(1 + 3, 20 + 2, 10 + 1);

    ASSERT_EQ(expected_sum.value(), pint::add_wrap(a, b).value());
}

TEST(TestAddWrap, WithOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(1, 60, 10);
    constexpr auto b = PackedInt(31, 20, 27);

    constexpr auto expected_sum = PackedInt(31 + 1, 60 + 20, 10 + 27);

    ASSERT_EQ(expected_sum.value(), pint::add_wrap(a, b).value());
}

TEST(TestAddWrap, WithOverflow2) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(3, 4, 5);
    constexpr auto b = PackedInt(5, 6, 7);

    constexpr auto expected_sum = PackedInt(3 + 5, 4 + 6, 5 + 7);

    ASSERT_EQ(expected_sum.value(), pint::add_wrap(a, b).value());
}

TEST(TestAddWrap, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 1);
    constexpr auto b = PackedInt(0, 0, 1);

    constexpr auto expected_sum = PackedInt(1,0,0);

    ASSERT_EQ(expected_sum.value(), pint::add_wrap(a, b).value());
}

TEST(TestAddUnsignedSaturate, EqualLength_NoOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum.value(), pint::add_unsigned_saturate(a, b).value());
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = PackedInt(7,6,7);

    ASSERT_EQ(expected_sum.value(), pint::add_unsigned_saturate(a, b).value());
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 1);
    constexpr auto b = PackedInt(0, 0, 1);

    constexpr auto expected_sum = PackedInt(1,0,1);

    ASSERT_EQ(expected_sum.value(), pint::add_unsigned_saturate(a, b).value());
}

TEST(TestAddUnsignedSaturate, VarLength_WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 4, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = PackedInt(7,6,7);

    ASSERT_EQ(expected_sum.value(), pint::add_unsigned_saturate(a, b).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, EqualLength_Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, EqualLength_Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-2, -3, -4);

    constexpr auto expected_sum = PackedInt(-1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, EqualLength_PositiveNegative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, -2, 3);
    constexpr auto b = PackedInt(-2, 3, -4);

    constexpr auto expected_sum = PackedInt(1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, EqualLength_Positive_Overflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    constexpr auto expected_sum = PackedInt(7, 6, 7);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, EqualLength_Negative_Overflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-8, -4, -6);

    constexpr auto expected_sum = PackedInt(-8, -6, -8);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, VarLength_Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, VarLength_Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-2, -3, -4);

    constexpr auto expected_sum = PackedInt(-1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, VarLength_PositiveNegative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, -2, 3);
    constexpr auto b = PackedInt(-2, 3, -4);

    constexpr auto expected_sum = PackedInt(1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, VarLength_Positive_Overflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, 10, 3);
    constexpr auto b = PackedInt(7, 14, 6);

    constexpr auto expected_sum = PackedInt(7, 15, 7);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

TEST(TestAddSignedSaturate, VarLength_Negative_Overflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(-1, -12, -3);
    constexpr auto b = PackedInt(-8, -14, -6);

    constexpr auto expected_sum = PackedInt(-8, -16, -8);

    ASSERT_EQ(expected_sum.value(), pint::add_signed_saturate(a, b).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubWrap, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff.value(), pint::sub_wrap(a, b).value());
}

TEST(TestSubWrap, NoOverflow2) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(7, 6, 5);
    constexpr auto b = PackedInt(1, 2, 3);

    constexpr auto expected_diff = PackedInt(7 - 1, 6 - 2, 5 - 3);

    ASSERT_EQ(expected_diff.value(), pint::sub_wrap(a, b).value());
}

TEST(TestSubWrap, NoOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 1, 0);
    constexpr auto b = PackedInt(1, 0, 0);

    constexpr auto expected_diff = PackedInt(1 - 1, 1 - 0, 0 - 0);

    ASSERT_EQ(expected_diff.value(), pint::sub_wrap(a, b).value());
}

TEST(TestSubWrap, WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 4, 2);
    constexpr auto b = PackedInt(7, 2, 6);

    constexpr auto expected_diff = PackedInt(1 - 7, 4 - 2, 2 - 6);

    ASSERT_EQ(expected_diff.value(), pint::sub_wrap(a, b).value());
}

TEST(TestSubWrap, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 0);
    constexpr auto b = PackedInt(1, 1, 0);

    constexpr auto expected_diff = PackedInt(1 - 1, 0 - 1, 0 - 0);

    ASSERT_EQ(expected_diff.value(), pint::sub_wrap(a, b).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubUnsignedSaturate, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff.value(), pint::sub_unsigned_saturate(a, b).value());
}

TEST(TestSubUnsignedSaturate, WithOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 2, 1);
    constexpr auto b = PackedInt(3, 20, 10);

    constexpr auto expected_diff = PackedInt(1, 0, 0);

    ASSERT_EQ(expected_diff.value(), pint::sub_unsigned_saturate(a, b).value());
}

TEST(TestSubUnsignedSaturate, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 0);
    constexpr auto b = PackedInt(1, 1, 0);

    constexpr auto expected_diff = PackedInt(0, 0, 0);

    ASSERT_EQ(expected_diff.value(), pint::sub_unsigned_saturate(a, b).value());
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubSignedSaturate, Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff.value(), pint::sub_signed_saturate(a, b).value());
}

TEST(TestSubSignedSaturate, Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(-4, -20, -10);
    constexpr auto b = PackedInt(-3, -2, -1);

    constexpr auto expected_diff = PackedInt(-4 - -3, -20 - -2, -10 - -1);

    ASSERT_EQ(expected_diff.value(), pint::sub_signed_saturate(a, b).value());
}

TEST(TestSubSignedSaturate, PositiveNegativeOverflow) {
    using PackedInt = pint::make_packed_int<4, 6, 4>;

    constexpr auto a = PackedInt(4, 0, 7);
    constexpr auto b = PackedInt(-6, -32, 1);

    constexpr auto expected_diff = PackedInt(7, 31, 6);

    ASSERT_EQ(expected_diff.value(), pint::sub_signed_saturate(a, b).value());
}

TEST(TestSubSignedSaturate, NegativePositiveOverflow) {
    using PackedInt = pint::make_packed_int<4, 6, 4>;

    constexpr auto a = PackedInt(-4, -2, -6);
    constexpr auto b = PackedInt(6, 30, 1);

    constexpr auto expected_diff = PackedInt(-8, -32, -7);

    ASSERT_EQ(expected_diff.value(), pint::sub_signed_saturate(a, b).value());
}
