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

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 5, 6, 5>(4, 22, 11);

    ASSERT_EQ(expected_sum, (pint::add_wrap<5,6,5>(a, b)));
}

TEST(TestAddWrap, WithOverflow) {
    constexpr auto a = pint::make_truncate<uint16_t, 5, 6, 5>(1, 60, 10);
    constexpr auto b = pint::make_truncate<uint16_t, 5, 6, 5>(31, 20, 27);

    constexpr auto expected_sum = pint::make_truncate<uint16_t, 5, 6, 5>(
        (31 + 1) & 0x1F, (60 + 20) & 0x3F, (10 + 27) & 0x1F
    );

    ASSERT_EQ(expected_sum, (pint::add_wrap<5,6,5>(a, b)));
}