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
