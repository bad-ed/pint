#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace pint {
namespace detail {

template<size_t value>
using size_t_ = std::integral_constant<size_t, value>;

template<class ...Types> struct seq {};
template<size_t ...Values>
using integer_seq = seq<size_t_<Values>...>;

// Prepend list<Types...> with type
template<class T, class Seq> struct prepend_seq_impl;
template<class T, class ...Types>
struct prepend_seq_impl<T, seq<Types...>> {
    using type = seq<T, Types...>;
};

template<class T, class Seq>
using prepend_seq = typename prepend_seq_impl<T, Seq>::type;

// Pop front from list<Types...>
template<class Seq> struct pop_front_impl;
template<class T, class ...Types>
struct pop_front_impl<seq<T, Types...>> {
    using type = seq<Types...>;
};
template<class Seq>
using pop_front = typename pop_front_impl<Seq>::type;

// Take first element from sequence
template<class Seq> struct take_1st_impl;
template<class First, class ...Others>
struct take_1st_impl<seq<First, Others...>> {
    using type = First;
};
template<class Seq> using take_1st = typename take_1st_impl<Seq>::type;

// Take second element from sequence
template<class Seq> struct take_2nd_impl;
template<class First, class Second, class ...Others>
struct take_2nd_impl<seq<First, Second, Others...>> {
    using type = Second;
};
template<class Seq> using take_2nd = typename take_2nd_impl<Seq>::type;

// Zip two sequences
template<class Seq0, class Seq1> struct zip_impl;
template<class ...Types0, class ...Types1>
struct zip_impl<seq<Types0...>, seq<Types1...>> {
    using type = seq<seq<Types0, Types1>...>;
};
template<class Seq0, class Seq1>
using zip = typename zip_impl<Seq0, Seq1>::type;

// Check if all elements in sequence are the same
template<class Seq> struct all_same;

template<> struct all_same<seq<>> : std::true_type {};
template<class First, class ...Others>
struct all_same<seq<First, Others...>> :
    std::is_same<seq<First, First, Others...>, seq<First, Others..., First>>::type
{};

// Defines type which is equal to integer_seq<Bits0, Bits0 + Bits1, Bits0 + Bits1 + Bits2, ...>
// Max is the maximum number of elements to process. Length of resulting vector is min(Max, sizeof...(Bits))
template<size_t Max, size_t Sum, size_t ...Bits> struct make_sum_vector_impl;

template<size_t Max, size_t Sum, size_t Bits0, size_t ...Bits>
struct make_sum_vector_impl<Max, Sum, Bits0, Bits...> {
    using type = prepend_seq<size_t_<Sum + Bits0>,
        typename make_sum_vector_impl<Max - 1, Sum + Bits0, Bits...>::type
    >;
};
template<size_t Sum, size_t Bits0, size_t ...Bits>
struct make_sum_vector_impl<0, Sum, Bits0, Bits...> {
    using type = integer_seq<>;
};
template<size_t Sum, size_t ...Bits>
struct make_sum_vector_impl<0, Sum, Bits...> {
    using type = integer_seq<>;
};
template<size_t ...Bits>
using make_sum_vector = typename make_sum_vector_impl<sizeof...(Bits), 0, Bits...>::type;
template<size_t N, size_t ...Bits>
using make_sum_vector_n = typename make_sum_vector_impl<N, 0, Bits...>::type;

// Sum of given bits
template<size_t ...Bits> struct sum;

#if __cpp_fold_expressions
template<size_t ...Bits> struct sum {
    static const size_t value = (... + Bits);
};
#else
template<size_t Bits0, size_t ...Bits>
struct sum<Bits0, Bits...> {
    static const size_t value = Bits0 + sum<Bits...>::value;
};
template<> struct sum<> {
    static const size_t value = 0;
};
#endif

// Vector of offsets
template<size_t... Bits>
using mask_offsets_vector = prepend_seq<
    size_t_<0>,
    make_sum_vector_n<sizeof...(Bits) - 1, Bits...> // (0, Bits0, Bits0 + Bits1, Bits0 + Bits1 + Bits2, ...)
>;

// Subtract from each element of vector value
template<class IntegerSeq, size_t Value> struct vector_sub_impl;
template<size_t Value, size_t ...Bits>
struct vector_sub_impl<integer_seq<Bits...>, Value>
{
    using type = integer_seq<(Bits - Value)...>;
};
template<class IntegerSeq, size_t Value>
using vector_sub = typename vector_sub_impl<IntegerSeq, Value>::type;

// Make mask which is equal to (1 << Bits0) | (1 << Bits1) | (1 << Bits2) | ...
template<class T, class IntegerSeq> struct make_mask_from_bitpos_impl;

#if __cpp_fold_expressions
template<class T, size_t ...Bits>
struct make_mask_from_bitpos_impl<T, integer_seq<Bits...>> {
    static const T value = (... | (1 << Bits));
};
#else
template<class T, size_t Bits0, size_t ...Bits>
struct make_mask_from_bitpos_impl<T, integer_seq<Bits0, Bits...>> {
    static const T value = (1 << Bits0) |
        make_mask_from_bitpos_impl<T, integer_seq<Bits...>>::value;
};
template<class T> struct make_mask_from_bitpos_impl<T, integer_seq<>> {
    static const T value = 0;
};
#endif

// Mask = (1 << (Bits0 - 1)) | (1 << (Bits0 + Bits1 - 1)) | (1 << (Bits0 + Bits1 + Bits2 - 1)) | ...
template<class T, size_t ...Bits>
using mask_hiorder = std::integral_constant<T,
    make_mask_from_bitpos_impl<T,
        vector_sub<make_sum_vector<Bits...>, 1>
    >::value
>;

// Make mask which is equal to (1 << 0) | (1 << Bits0) | (1 << Bits0 + Bits1) | (1 << Bits0 + Bits1 + Bits2) | ...
template<class T, size_t ...Bits>
using mask_loorder = std::integral_constant<T,
    make_mask_from_bitpos_impl<T,
        mask_offsets_vector<Bits...>
    >::value
>;

// Make mask of length Bits with all bits set to 1
template<class T, size_t Bits> struct all_ones {
    static const T value = (1 << Bits) - 1;
};
template<class T> struct all_ones<T, 32> {
    static const T value = static_cast<T>(0xffffffff);
};
template<class T> struct all_ones<T, 64> {
    static const T value = static_cast<T>(~0ULL);
};

// Make sequence of pairs <offset of mask, mask length>
template<size_t... Bits>
using offset_and_mask_vector = zip<
    mask_offsets_vector<Bits...>,
    zip<integer_seq<Bits...>, integer_seq<Bits...>>
>;
template<class Integer, class OffsetAndMask, template <class> class mask_getter>
using shifted_mask = std::integral_constant<Integer, (
    all_ones<Integer, mask_getter<take_2nd<OffsetAndMask>>::value>::value <<
        take_1st<OffsetAndMask>::value
)>;

template<class Integer>
constexpr Integer carry_add_vector(Integer a, Integer b) {
    return (a & b) | ((a | b) & ~(a + b));
}

template<class Integer>
constexpr Integer carry_sub_vector(Integer a, Integer b) {
    return (~a & b) | ((~(a ^ b)) & (a - b));
}

template<class Integer>
constexpr Integer overflow_signed_sub_vector(Integer a, Integer b, Integer res) {
    return (~a & b & res) | (a & ~(b | res));
}

#if __cpp_fold_expressions
template<class Integer, class ...Values, class ...MasksAndOffsets>
constexpr Integer make_truncated_int(seq<MasksAndOffsets...>, Values ...values)
{
    return (... | ((values & all_ones<Integer, take_1st<take_2nd<MasksAndOffsets>>::value>::value) << take_1st<MasksAndOffsets>::value));
}
#else
template<class Integer> constexpr Integer make_truncated_int(seq<>) { return 0; }
template<class Integer, class ...Values, class ...MasksAndOffsets>
constexpr Integer make_truncated_int(seq<MasksAndOffsets...>, Integer value0, Values ...values)
{
    using my_mask_and_offset = take_1st<seq<MasksAndOffsets...>>;

    return ((value0 & all_ones<Integer, take_1st<take_2nd<my_mask_and_offset>>::value>::value) <<
            take_1st<my_mask_and_offset>::value)
        | make_truncated_int<Integer>(pop_front<seq<MasksAndOffsets...>>(), values...);
}
#endif

// Unsigned sum with saturation
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_unsigned_saturate(
    Integer sum, Integer carrys, std::true_type /* all bits are the same */)
{
    return sum | ((carrys << 1) - (carrys >> (Bits0 - 1)));
}

#if __cpp_fold_expressions
template<class ...OffsetAndMasks, class Integer>
constexpr Integer add_unsigned_saturate2(Integer carrys, seq<OffsetAndMasks...>) {
    return (... |
        (carrys & shifted_mask<Integer, OffsetAndMasks, take_1st>::value
            ? shifted_mask<Integer, OffsetAndMasks, take_2nd>::value
            : 0));
}
#else
template<class Integer>
constexpr Integer add_unsigned_saturate2(Integer carrys, seq<>) {
    return 0;
}

template<class ...OffsetAndMasks, class Integer>
constexpr Integer add_unsigned_saturate2(
    Integer carrys, seq<OffsetAndMasks...>)
{
    using offset_and_mask = take_1st<seq<OffsetAndMasks...>>;

    return (carrys & shifted_mask<Integer, offset_and_mask, take_1st>::value ? shifted_mask<Integer, offset_and_mask, take_2nd>::value : 0) |
        add_unsigned_saturate2(carrys, pop_front<seq<OffsetAndMasks...>>());
}
#endif

template<size_t ...Bits, class Integer>
constexpr Integer add_unsigned_saturate(
    Integer sum, Integer carrys, std::false_type /* packs of variable length */)
{
    return sum | add_unsigned_saturate2(carrys, offset_and_mask_vector<Bits...>());
}

// Signed sum with saturation
template<size_t BitmaskLen, class Integer>
Integer signed_payload_bits_from_overflow(Integer overflow)
{
    return overflow - (overflow >> (BitmaskLen-1));
}

template<size_t Bits0, class Integer>
constexpr Integer add_signed_saturate_same_length(Integer sum, Integer overflow)
{
    return sum ^ signed_payload_bits_from_overflow<Bits0>(sum & overflow);
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer apply_signed_saturation(
    Integer sum, Integer overflow, std::true_type /* packs of same length */)
{
    return add_signed_saturate_same_length<Bits0>(static_cast<Integer>(
        (sum ^ overflow) | signed_payload_bits_from_overflow<Bits0>(overflow)), overflow);
}

template<class Integer, class MasksAndOffsets>
constexpr Integer apply_signed_saturate_var_len2(Integer sum, Integer overflow, MasksAndOffsets masks_and_offsets)
{
    return sum ^ add_unsigned_saturate2(overflow & sum, masks_and_offsets);
}

template<size_t ...Bits, class Integer>
constexpr Integer apply_signed_saturate_var_len(Integer sum, Integer overflow)
{
    using masks_and_offsets = zip<
        mask_offsets_vector<Bits...>,
        zip<integer_seq<Bits...>, integer_seq<(Bits-1)...>>
    >;

    return apply_signed_saturate_var_len2<Integer>(
        (sum ^ overflow) | add_unsigned_saturate2(overflow, masks_and_offsets()),
        overflow,
        masks_and_offsets());
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer apply_signed_saturation(
    Integer sum, Integer overflow, std::false_type /* packs of different lengths */)
{
    return apply_signed_saturate_var_len<Bits0, Bits...>(sum, overflow);
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_signed_saturate(Integer a, Integer b, Integer sum)
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return apply_signed_saturation<Bits0, Bits...>(
        sum,
        static_cast<Integer>((~(a ^ b)) & (sum ^ b) & mask2::value),
        detail::all_same<detail::integer_seq<Bits0, Bits...>>());
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer sub_signed_saturate(Integer a, Integer b, Integer diff)
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return apply_signed_saturation<Bits0, Bits...>(
        diff,
        static_cast<Integer>(overflow_signed_sub_vector(a, b, diff) & mask2::value),
        detail::all_same<detail::integer_seq<Bits0, Bits...>>());
}

template<class Integer, size_t Bits0, size_t ...Bits>
static constexpr Integer make_truncate(Integer value0,
    typename std::integral_constant<Integer, Bits>::value_type ...values) noexcept
{
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integral won't fit given number of bits");

    using offset_and_mask = detail::offset_and_mask_vector<Bits0, Bits...>;
    return detail::make_truncated_int<Integer>(offset_and_mask(), value0, values...);
}

template<size_t RequiredBits> struct find_appropriate_int;
template<> struct find_appropriate_int<8> { using type = uint8_t; };
template<> struct find_appropriate_int<16> { using type = uint16_t; };
template<> struct find_appropriate_int<32> { using type = uint32_t; };
template<> struct find_appropriate_int<64> { using type = uint64_t; };

} // namespace detail

template<class Integer, size_t Bits0, size_t ...Bits>
class packed_int {
public:
    static_assert(std::is_integral<Integer>::value && std::is_unsigned<Integer>::value,
        "Integer must be unsigned integer");
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integer won't fit given number of bits");

    using value_type = Integer;

    constexpr explicit packed_int(value_type value) noexcept : m_value(value) {}

    template<size_t BitCount = sizeof...(Bits), typename std::enable_if<BitCount != 0, int>::type = 0>
    constexpr packed_int(value_type value0,
        typename std::integral_constant<value_type, Bits>::value_type ...values) noexcept
        : m_value(detail::make_truncate<value_type, Bits0, Bits...>(value0, values...))
    {}

    constexpr value_type value() const { return m_value; }

private:
    value_type m_value;
};

template<size_t Bits0, size_t ...Bits>
using make_packed_int = packed_int<
    typename detail::find_appropriate_int<
        (detail::sum<Bits0, Bits...>::value + 7) & ~7
    >::type,
    Bits0, Bits...
>;

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_wrap(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    using mask1 = std::integral_constant<Integer, (~mask2::value)
        & detail::all_ones<Integer, detail::sum<Bits0, Bits...>::value>::value>;

    return packed_int<Integer, Bits0, Bits...>(
        ((a.value() & mask1::value) + (b.value() & mask1::value)) ^
        ((a.value() ^ b.value()) & mask2::value));
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_unsigned_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;

    return packed_int<Integer, Bits0, Bits...>(
        detail::add_unsigned_saturate<Bits0, Bits...>(
            add_wrap(a, b).value(), // potentially overflown result
            static_cast<Integer>(detail::carry_add_vector(a.value(), b.value()) & mask2::value), // carry vector
            detail::all_same<detail::integer_seq<Bits0, Bits...>>())
    );
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_signed_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    return packed_int<Integer, Bits0, Bits...>(
        detail::add_signed_saturate<Bits0, Bits...>(
            a.value(), b.value(), add_wrap(a, b).value())
    );
}

///////////////////////////////////////////////////////////////////////////////

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_wrap(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using mask3 = detail::mask_loorder<Integer, Bits0, Bits...>;
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    using mask1 = std::integral_constant<Integer, (~mask2::value)
        & detail::all_ones<Integer, detail::sum<Bits0, Bits...>::value>::value>;

    return packed_int<Integer, Bits0, Bits...>(
        ((a.value() & mask1::value) + (~b.value() & mask1::value) + (mask3::value & mask1::value)) ^
        ((a.value() ^ ~b.value()) & mask2::value) ^ (mask2::value & mask3::value)
    );
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_unsigned_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using packed = packed_int<Integer, Bits0, Bits...>;
    using mask3 = detail::mask_loorder<Integer, Bits0, Bits...>;
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;

    // a + ~b with saturation (using carry subtraction vector),
    // then add mask with low order bits with overflow
    return add_wrap(
        packed(detail::add_unsigned_saturate<Bits0, Bits...>(
            add_wrap(a, packed(~b.value())).value(), // potentially overflown result
            static_cast<Integer>(detail::carry_sub_vector(a.value(), b.value()) & mask2::value), // overflow vector
            detail::all_same<detail::integer_seq<Bits0, Bits...>>())),
        packed(mask3::value));
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_signed_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    return packed_int<Integer, Bits0, Bits...>(
        detail::sub_signed_saturate<Bits0, Bits...>(
            a.value(), b.value(), sub_wrap(a, b).value())
    );
}

} // namespace pint
