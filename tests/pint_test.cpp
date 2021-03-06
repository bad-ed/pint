#include <array>
#include <utility>

#include <gtest/gtest.h>
#include "pint/pint.hpp"

#if __cpp_lib_integer_sequence
template<size_t ...Indexes> using IndexSeq = std::index_sequence<Indexes...>;
template<size_t N> using MakeIndexSeq = std::make_index_sequence<N>;
#else
template<size_t ...Indexes> using IndexSeq = pint::detail::integer_seq<Indexes...>;
template<size_t N, class IntegerSeq> struct Increase;
template<size_t N, size_t ...Values>
struct Increase<N, pint::detail::integer_seq<Values...>> {
    using type = pint::detail::integer_seq<(Values + N)...>;
};
template<size_t N>
struct MakeIndexSeqImpl {
    using L1 = typename MakeIndexSeqImpl<N / 2>::type;
    using L2 = typename Increase<N / 2, L1>::type;
    using L3 = pint::detail::repeat<pint::detail::size_t_<N-1>, N % 2>;

    using type = pint::detail::concat<
        pint::detail::concat<L1, L2>, L3
    >;
};
template<> struct MakeIndexSeqImpl<0> { using type = pint::detail::seq<>; };
template<size_t N>
using MakeIndexSeq = typename MakeIndexSeqImpl<N>::type;
#endif

template<class Integer, class PackedInt, size_t ...Indexes>
std::array<Integer, sizeof...(Indexes)>
    ToArrayHelper(PackedInt value, IndexSeq<Indexes...>)
{
    return std::array<Integer, sizeof...(Indexes)>{pint::get<Indexes>(value)...};
}

template<size_t Bits0, size_t ...Bits, class Integer>
std::array<Integer, sizeof...(Bits) + 1>
    ToArray(pint::packed_int<Integer, Bits0, Bits...> value)
{
    return ToArrayHelper<Integer>(value, MakeIndexSeq<sizeof...(Bits)+1>());
}

namespace pint {
template<size_t Bits0, size_t ...Bits, class Integer>
void PrintTo(packed_int<Integer, Bits0, Bits...> value, std::ostream* os) {
    const auto packed_values = ToArray(value);

    *os << value.value() << '{' << packed_values[0];
    for (size_t i = 1; i < packed_values.size(); ++i)
        *os << ',' << packed_values[i];
    *os << '}';
}
} // namespace pint

template<size_t BitsSum>
using MakePackedIntValueType = typename pint::make_packed_int<BitsSum>::value_type;

static_assert(std::is_same<MakePackedIntValueType<1>, uint8_t>::value,
    "Value type must be uint8_t");
static_assert(std::is_same<MakePackedIntValueType<7>, uint8_t>::value,
    "Value type must be uint8_t");
static_assert(std::is_same<MakePackedIntValueType<8>, uint8_t>::value,
    "Value type must be uint8_t");

static_assert(std::is_same<MakePackedIntValueType<9>, uint16_t>::value,
    "Value type must be uint16_t");
static_assert(std::is_same<MakePackedIntValueType<15>, uint16_t>::value,
    "Value type must be uint16_t");
static_assert(std::is_same<MakePackedIntValueType<16>, uint16_t>::value,
    "Value type must be uint16_t");

static_assert(std::is_same<MakePackedIntValueType<17>, uint32_t>::value,
    "Value type must be uint32_t");
static_assert(std::is_same<MakePackedIntValueType<31>, uint32_t>::value,
    "Value type must be uint32_t");
static_assert(std::is_same<MakePackedIntValueType<32>, uint32_t>::value,
    "Value type must be uint32_t");

static_assert(std::is_same<MakePackedIntValueType<33>, uint64_t>::value,
    "Value type must be uint64_t");
static_assert(std::is_same<MakePackedIntValueType<63>, uint64_t>::value,
    "Value type must be uint64_t");
static_assert(std::is_same<MakePackedIntValueType<64>, uint64_t>::value,
    "Value type must be uint64_t");

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

    ASSERT_EQ(sliced, SlicedInt(3,4));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddWrap, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(1, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_sum = PackedInt(1 + 3, 20 + 2, 10 + 1);

    ASSERT_EQ(expected_sum, pint::add_wrap(a, b));
}

TEST(TestAddWrap, WithOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(1, 60, 10);
    constexpr auto b = PackedInt(31, 20, 27);

    constexpr auto expected_sum = PackedInt(31 + 1, 60 + 20, 10 + 27);

    ASSERT_EQ(expected_sum, pint::add_wrap(a, b));
}

TEST(TestAddWrap, WithOverflow2) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(3, 4, 5);
    constexpr auto b = PackedInt(5, 6, 7);

    constexpr auto expected_sum = PackedInt(3 + 5, 4 + 6, 5 + 7);

    ASSERT_EQ(expected_sum, pint::add_wrap(a, b));
}

TEST(TestAddWrap, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 1);
    constexpr auto b = PackedInt(0, 0, 1);

    constexpr auto expected_sum = PackedInt(1,0,0);

    ASSERT_EQ(expected_sum, pint::add_wrap(a, b));
}

TEST(TestAddUnsignedSaturate, EqualLength_NoOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, pint::add_unsigned_saturate(a, b));
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = PackedInt(7,6,7);

    ASSERT_EQ(expected_sum, pint::add_unsigned_saturate(a, b));
}

TEST(TestAddUnsignedSaturate, EqualLength_WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 1);
    constexpr auto b = PackedInt(0, 0, 1);

    constexpr auto expected_sum = PackedInt(1,0,1);

    ASSERT_EQ(expected_sum, pint::add_unsigned_saturate(a, b));
}

TEST(TestAddUnsignedSaturate, VarLength_WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 4, 3>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    // 1 + 7 == 8 saturates to 7, 3 + 6 == 9 saturates to 7
    constexpr auto expected_sum = PackedInt(7,6,7);

    ASSERT_EQ(expected_sum, pint::add_unsigned_saturate(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, EqualLength_Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, EqualLength_Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-2, -3, -4);

    constexpr auto expected_sum = PackedInt(-1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, EqualLength_PositiveNegative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, -2, 3);
    constexpr auto b = PackedInt(-2, 3, -4);

    constexpr auto expected_sum = PackedInt(1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, EqualLength_Positive_Overflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(7, 4, 6);

    constexpr auto expected_sum = PackedInt(7, 6, 7);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, EqualLength_Negative_Overflow) {
    using PackedInt = pint::make_packed_int<4, 4, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-8, -4, -6);

    constexpr auto expected_sum = PackedInt(-8, -6, -8);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestAddSignedSaturate, VarLength_Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, 2, 3);
    constexpr auto b = PackedInt(2, 3, 4);

    constexpr auto expected_sum = PackedInt(1 + 2, 2 + 3, 3 + 4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, VarLength_Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(-1, -2, -3);
    constexpr auto b = PackedInt(-2, -3, -4);

    constexpr auto expected_sum = PackedInt(-1 + -2, -2 + -3, -3 + -4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, VarLength_PositiveNegative_NoOverflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, -2, 3);
    constexpr auto b = PackedInt(-2, 3, -4);

    constexpr auto expected_sum = PackedInt(1 + -2, -2 + 3, 3 + -4);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, VarLength_Positive_Overflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(1, 10, 3);
    constexpr auto b = PackedInt(7, 14, 6);

    constexpr auto expected_sum = PackedInt(7, 15, 7);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

TEST(TestAddSignedSaturate, VarLength_Negative_Overflow) {
    using PackedInt = pint::make_packed_int<4, 5, 4>;

    constexpr auto a = PackedInt(-1, -12, -3);
    constexpr auto b = PackedInt(-8, -14, -6);

    constexpr auto expected_sum = PackedInt(-8, -16, -8);

    ASSERT_EQ(expected_sum, pint::add_signed_saturate(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubWrap, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, pint::sub_wrap(a, b));
}

TEST(TestSubWrap, NoOverflow2) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(7, 6, 5);
    constexpr auto b = PackedInt(1, 2, 3);

    constexpr auto expected_diff = PackedInt(7 - 1, 6 - 2, 5 - 3);

    ASSERT_EQ(expected_diff, pint::sub_wrap(a, b));
}

TEST(TestSubWrap, NoOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 1, 0);
    constexpr auto b = PackedInt(1, 0, 0);

    constexpr auto expected_diff = PackedInt(1 - 1, 1 - 0, 0 - 0);

    ASSERT_EQ(expected_diff, pint::sub_wrap(a, b));
}

TEST(TestSubWrap, WithOverflow) {
    using PackedInt = pint::make_packed_int<3, 3, 3>;

    constexpr auto a = PackedInt(1, 4, 2);
    constexpr auto b = PackedInt(7, 2, 6);

    constexpr auto expected_diff = PackedInt(1 - 7, 4 - 2, 2 - 6);

    ASSERT_EQ(expected_diff, pint::sub_wrap(a, b));
}

TEST(TestSubWrap, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 0);
    constexpr auto b = PackedInt(1, 1, 0);

    constexpr auto expected_diff = PackedInt(1 - 1, 0 - 1, 0 - 0);

    ASSERT_EQ(expected_diff, pint::sub_wrap(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubUnsignedSaturate, NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, pint::sub_unsigned_saturate(a, b));
}

TEST(TestSubUnsignedSaturate, WithOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 2, 1);
    constexpr auto b = PackedInt(3, 20, 10);

    constexpr auto expected_diff = PackedInt(1, 0, 0);

    ASSERT_EQ(expected_diff, pint::sub_unsigned_saturate(a, b));
}

TEST(TestSubUnsignedSaturate, WithOverflow_1BitPacks) {
    using PackedInt = pint::make_packed_int<1, 1, 1>;

    constexpr auto a = PackedInt(1, 0, 0);
    constexpr auto b = PackedInt(1, 1, 0);

    constexpr auto expected_diff = PackedInt(0, 0, 0);

    ASSERT_EQ(expected_diff, pint::sub_unsigned_saturate(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestSubSignedSaturate, Positive_NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(4, 20, 10);
    constexpr auto b = PackedInt(3, 2, 1);

    constexpr auto expected_diff = PackedInt(4 - 3, 20 - 2, 10 - 1);

    ASSERT_EQ(expected_diff, pint::sub_signed_saturate(a, b));
}

TEST(TestSubSignedSaturate, Negative_NoOverflow) {
    using PackedInt = pint::make_packed_int<5, 6, 5>;

    constexpr auto a = PackedInt(-4, -20, -10);
    constexpr auto b = PackedInt(-3, -2, -1);

    constexpr auto expected_diff = PackedInt(-4 - -3, -20 - -2, -10 - -1);

    ASSERT_EQ(expected_diff, pint::sub_signed_saturate(a, b));
}

TEST(TestSubSignedSaturate, PositiveNegativeOverflow) {
    using PackedInt = pint::make_packed_int<4, 6, 4>;

    constexpr auto a = PackedInt(4, 0, 7);
    constexpr auto b = PackedInt(-6, -32, 1);

    constexpr auto expected_diff = PackedInt(7, 31, 6);

    ASSERT_EQ(expected_diff, pint::sub_signed_saturate(a, b));
}

TEST(TestSubSignedSaturate, NegativePositiveOverflow) {
    using PackedInt = pint::make_packed_int<4, 6, 4>;

    constexpr auto a = PackedInt(-4, -2, -6);
    constexpr auto b = PackedInt(6, 30, 1);

    constexpr auto expected_diff = PackedInt(-8, -32, -7);

    ASSERT_EQ(expected_diff, pint::sub_signed_saturate(a, b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestMinUnsigned, AllFirstLessThanSecond) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(1,2,3);
    constexpr auto b = PackedInt(4,5,15);

    constexpr auto expected_min = PackedInt(1,2,3);

    ASSERT_EQ(expected_min, pint::min_unsigned(a,b));
}

TEST(TestMinUnsigned, AllSecondLessThanFirst) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(4,5,15);
    constexpr auto b = PackedInt(1,2,3);

    constexpr auto expected_min = PackedInt(1,2,3);

    ASSERT_EQ(expected_min, pint::min_unsigned(a,b));
}

TEST(TestMinUnsigned, Interleaved) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(4,5,3);
    constexpr auto b = PackedInt(1,15,3);

    constexpr auto expected_min = PackedInt(1,5,3);

    ASSERT_EQ(expected_min, pint::min_unsigned(a,b));
}

TEST(TestMaxUnsigned, AllFirstLessThanSecond) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(1,2,3);
    constexpr auto b = PackedInt(4,5,15);

    constexpr auto expected_max = PackedInt(4,5,15);

    ASSERT_EQ(expected_max, pint::max_unsigned(a,b));
}

TEST(TestMaxUnsigned, AllSecondLessThanFirst) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(4,5,15);
    constexpr auto b = PackedInt(1,2,3);

    constexpr auto expected_max = PackedInt(4,5,15);

    ASSERT_EQ(expected_max, pint::max_unsigned(a,b));
}

TEST(TestMaxUnsigned, Interleaved) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(4,5,3);
    constexpr auto b = PackedInt(1,15,3);

    constexpr auto expected_max = PackedInt(4,15,3);

    ASSERT_EQ(expected_max, pint::max_unsigned(a,b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestMinSigned, NegativeNegative) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(-1,-5,0);
    constexpr auto b = PackedInt(-4,-2,-8);

    constexpr auto expected_min = PackedInt(-4,-5,-8);

    ASSERT_EQ(expected_min, pint::min_signed(a,b));
}

TEST(TestMinSigned, PositivePositive) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(1,5,0);
    constexpr auto b = PackedInt(4,2,7);

    constexpr auto expected_min = PackedInt(1,2,0);

    ASSERT_EQ(expected_min, pint::min_signed(a,b));
}

TEST(TestMinSigned, PositiveNegative) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(-1,5,0);
    constexpr auto b = PackedInt(4,-2,7);

    constexpr auto expected_min = PackedInt(-1,-2,0);

    ASSERT_EQ(expected_min, pint::min_signed(a,b));
}

TEST(TestMaxSigned, NegativeNegative) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(-1,-5,0);
    constexpr auto b = PackedInt(-4,-2,-8);

    constexpr auto expected_max = PackedInt(-1,-2,0);

    ASSERT_EQ(expected_max, pint::max_signed(a,b));
}

TEST(TestMaxSigned, PositivePositive) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(1,5,0);
    constexpr auto b = PackedInt(4,2,7);

    constexpr auto expected_max = PackedInt(4,5,7);

    ASSERT_EQ(expected_max, pint::max_signed(a,b));
}

TEST(TestMaxSigned, PositiveNegative) {
    using PackedInt = pint::make_packed_int<4,6,4>;

    constexpr auto a = PackedInt(-1,5,0);
    constexpr auto b = PackedInt(4,-2,7);

    constexpr auto expected_max = PackedInt(4,5,7);

    ASSERT_EQ(expected_max, pint::max_signed(a,b));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestShiftLeft, SameLength_ShiftNotExceed)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(4,8,12);

    ASSERT_EQ(expected_value, shift_left(value, 2));
}

TEST(TestShiftLeft, SameLength_ShiftExceedPartially)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(8,0,8);

    ASSERT_EQ(expected_value, shift_left(value, 3));
}

TEST(TestShiftLeft, SameLength_ShiftExceed)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(0,0,0);

    ASSERT_EQ(expected_value, shift_left(value, 4));
}

TEST(TestShiftLeft, SameLength_ShiftExceedBits)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(0,0,0);

    // Use volatile here, to avoid compiler optimizations.
    // It gave false-positive result at first (when implementation was flawed).
    const volatile size_t shift = 5;
    ASSERT_EQ(expected_value, shift_left(value, shift));
}

TEST(TestShiftLeft, VarLength_ShiftNotExceed)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(4,8,12);

    ASSERT_EQ(expected_value, shift_left(value, 2));
}

TEST(TestShiftLeft, VarLength_ShiftExceedPartially)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(0,16,24);

    const volatile size_t shift = 3;
    ASSERT_EQ(expected_value, shift_left(value, shift));
}

TEST(TestShiftLeft, VarLength_ShiftExceed)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(1,2,3);
    constexpr auto expected_value = PackedInt(0,0,0);

    const volatile size_t shift = 6;
    ASSERT_EQ(expected_value, shift_left(value, shift));
}

//////////////////////////////////////////////////////////////////////////////

TEST(TestShiftRight, SameLength_ShiftNotExceed)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(4,8,12);
    constexpr auto expected_value = PackedInt(1,2,3);

    ASSERT_EQ(expected_value, shift_right_unsigned(value, 2));
}

TEST(TestShiftRight, SameLength_ShiftExceedPartially)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(4,8,12);
    constexpr auto expected_value = PackedInt(0,1,1);

    ASSERT_EQ(expected_value, shift_right_unsigned(value, 3));
}

TEST(TestShiftRight, SameLength_ShiftExceed)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(4,8,12);
    constexpr auto expected_value = PackedInt(0,0,0);

    ASSERT_EQ(expected_value, shift_right_unsigned(value, 4));
}

TEST(TestShiftRight, SameLength_ShiftExceedBits)
{
    using PackedInt = pint::make_packed_int<4,4,4>;

    constexpr auto value = PackedInt(4,8,12);
    constexpr auto expected_value = PackedInt(0,0,0);

    // Use volatile here, to avoid compiler optimizations.
    // It gave false-positive result at first (when implementation was flawed).
    const volatile size_t shift = 5;
    ASSERT_EQ(expected_value, shift_right_unsigned(value, shift));
}

TEST(TestShiftRight, VarLength_ShiftNotExceed)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(4,8,12);
    constexpr auto expected_value = PackedInt(1,2,3);

    ASSERT_EQ(expected_value, shift_right_unsigned(value, 2));
}

TEST(TestShiftRight, VarLength_ShiftExceedPartially)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(5,106,42);
    constexpr auto expected_value = PackedInt(0,6,2);

    const volatile size_t shift = 4;
    ASSERT_EQ(expected_value, shift_right_unsigned(value, shift));
}

TEST(TestShiftRight, VarLength_ShiftExceed)
{
    using PackedInt = pint::make_packed_int<3,7,6>;

    constexpr auto value = PackedInt(5,106,42);
    constexpr auto expected_value = PackedInt(0,1,0);

    const volatile size_t shift = 6;
    ASSERT_EQ(expected_value, shift_right_unsigned(value, shift));
}
