#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace pint {

using std::size_t;

template<class Integer, size_t Bits0, size_t ...Bits> class packed_int;

namespace detail {

template<class Type>
struct identity { using type = Type; };

template<size_t value>
using size_t_ = std::integral_constant<size_t, value>;

template<class ...Types> struct seq {};
template<size_t ...Values>
using integer_seq = seq<size_t_<Values>...>;

// Concat two sequences together
template<class Seq1, class Seq2> struct concat_impl;
template<class ...Types1, class ...Types2>
struct concat_impl<seq<Types1...>, seq<Types2...>> {
    using type = seq<Types1..., Types2...>;
};
template<class Seq1, class Seq2>
using concat = typename concat_impl<Seq1, Seq2>::type;

// Create sequence of Type repeated N times
template<class Type, size_t N>
struct repeat_impl {
    using l1 = typename repeat_impl<Type, N / 2>::type;
    using l2 = typename repeat_impl<Type, N % 2>::type;

    using type = concat<concat<l1, l1>, l2>;
};
template<class Type> struct repeat_impl<Type, 1> { using type = seq<Type>; };
template<class Type> struct repeat_impl<Type, 0> { using type = seq<>; };
template<class Type, size_t N> using repeat = typename repeat_impl<Type, N>::type;

// Prepend list<Types...> with type
template<class T, class Seq> struct prepend_seq_impl;
template<class T, class ...Types>
struct prepend_seq_impl<T, seq<Types...>> {
    using type = seq<T, Types...>;
};

template<class T, class Seq>
using prepend_seq = typename prepend_seq_impl<T, Seq>::type;

// Append type to sequence
template<class T, class Seq> struct append_seq_impl;
template<class T, class ...Types>
struct append_seq_impl<T, seq<Types...>> {
    using type = seq<Types..., T>;
};

template<class T, class Seq>
using append_seq = typename append_seq_impl<T, Seq>::type;

// Inherit from given classes
template<class ...Types> struct inherit : Types... {};

// Check if type is in set (set is a sequence with all elements unique)
template<class Type, class Seq> struct in_set_impl;
template<class Type, class ...Types>
struct in_set_impl<Type, seq<Types...>> {
    static std::true_type deduce(identity<Type>);
    static std::false_type deduce(...);

    using set = inherit<identity<Types>...>;
    using type = decltype(deduce(set()));
};
template<class Type, class Seq>
using in_set = typename in_set_impl<Type, Seq>::type;

// Remove duplicated elements from seq
template<class Seq, class State = seq<>> struct unique_impl;
template<class Type, class ...Types, class State>
struct unique_impl<seq<Type, Types...>, State> {
    using NewState = typename std::conditional<
        in_set<Type, State>::value,
        State,
        append_seq<Type, State>
    >::type;
    using type = typename unique_impl<seq<Types...>, NewState>::type;
};
template<class State>
struct unique_impl<seq<>, State> { using type = State; };
template<class Seq>
using unique = typename unique_impl<Seq>::type;

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

// Pop front from list<Types...>
template<class Seq> struct pop_front_impl;
template<class T, class ...Types>
struct pop_front_impl<seq<T, Types...>> {
    using type = seq<Types...>;
};
template<class Seq>
using pop_front = typename pop_front_impl<Seq>::type;

// Pop first N types from sequence
template<class Seq, class DummySeq> struct pop_front_n_impl;
template<class ...Types1, class ...Types2>
struct pop_front_n_impl<seq<Types1...>, seq<Types2...>> {
    template<class ...U>
    static seq<U...> deduce(Types2*..., identity<U>*...);
    using type = decltype(deduce(static_cast<identity<Types1>*>(nullptr)...));
};
template<size_t N, class Seq>
using pop_front_n = typename pop_front_n_impl<Seq, repeat<void, N>>::type;

// Make sequence of first N types of given sequence
template<size_t N, class Seq> struct take_front_n_impl {
    using type = prepend_seq<
        take_1st<Seq>, typename take_front_n_impl<N-1, pop_front<Seq>>::type
    >;
};
template<class Seq> struct take_front_n_impl<0, Seq> {
    using type = seq<>;
};
template<size_t N, class Seq>
using take_front_n = typename take_front_n_impl<N,Seq>::type;

// Take Nth element from sequence
template<size_t Index, class Seq> struct take_nth_impl {
    using type = take_1st<pop_front_n<Index, Seq>>;
};
template<class Seq> struct take_nth_impl<0, Seq> {
    using type = take_1st<Seq>;
};

template<size_t Index, class Seq>
using take_nth = typename take_nth_impl<Index, Seq>::type;

// Slice sequence
template<size_t First, size_t Last, class Seq>
using slice = take_front_n<Last-First, pop_front_n<First, Seq>>;

// Zip two sequences
template<class Seq0, class Seq1> struct zip_impl;
template<class ...Types0, class ...Types1>
struct zip_impl<seq<Types0...>, seq<Types1...>> {
    using type = seq<seq<Types0, Types1>...>;
};
template<class Seq0, class Seq1>
using zip = typename zip_impl<Seq0, Seq1>::type;

// Given sequence seq<seq<Key0, Value0>, seq<Key1, Value1>, ...>,
// return sequence of values whose corresponding keys are equal to Key
template<class Key, class Seq> struct filter_map_by_key_impl { using type = Seq; };
template<class FirstKey, class FirstValue, class ...Rest, class Key>
struct filter_map_by_key_impl<Key, seq<seq<FirstKey, FirstValue>, Rest...>> {
    using step_result = typename std::conditional<
        std::is_same<Key, FirstKey>::value,
        seq<FirstValue>,
        seq<>
    >::type;

    using type = concat<
        step_result,
        typename filter_map_by_key_impl<Key, seq<Rest...>>::type
    >;
};
template<class Key, class Seq>
using filter_map_by_key = typename filter_map_by_key_impl<Key, Seq>::type;

// Given sequence seq<seq<Key0, Value0>, seq<Key1, Value1>, ...>,
// return seq<Key0, Key1, ...>
template<class Seq> struct map_keys_impl;
template<class ...Seqs>
struct map_keys_impl<seq<Seqs...>> {
    using type = seq<take_1st<Seqs>...>;
};
template<class Seq> using map_keys = typename map_keys_impl<Seq>::type;

// Unzips sequence seq<seq<Key0, Value0>, seq<Key1, Value1>, ...> to
// seq<seq<KeyX, seq<ValueX0, ValueX1, ...>> >
template<class Keys, class Seq> struct unzip_to_map_impl;
template<class ...Keys, class Seq>
struct unzip_to_map_impl<seq<Keys...>, Seq> {
    using type = seq<
        seq<Keys, filter_map_by_key<Keys, Seq>>...
    >;
};
template<class Seq>
using unzip_to_map = typename unzip_to_map_impl<unique<map_keys<Seq>>, Seq>::type;

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

template<class Seq> struct sum_seq;
template<size_t ...Bits> struct sum_seq<integer_seq<Bits...>> : sum<Bits...> {};

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

// Calculate number set bits in value
template<uint64_t number>
struct bit_count : size_t_<1 + bit_count<number & (number - 1)>::value> {};
template<> struct bit_count<0> : size_t_<0> {};

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

template<size_t Index, size_t ...Bits>
using take_offset_and_mask = take_nth<Index,
    zip<
        mask_offsets_vector<Bits...>,
        integer_seq<Bits...>
    >
>;

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

/////////////////////////////////////////////////////////////////////
// Make unsigned saturation mask
// For example if carry vector is | 100 | 0000 | 100000 | 10 | 000 |, function returns
//                                | 111 | 0000 | 111111 | 11 | 000 |

// First detect how saturation mask is calculated from carry-out vector.
// There are 3 possible options:
// 1. If all bits in packed integer are the same, then saturation mask is
//    (carry << 1) - (carry >> (Bits0 - 1))
// 2. If bitcount((mask_hiorder >> (Bits0 - 1)) & mask_loorder)
//     + bitcount((mask_hiorder >> (Bits1 - 1)) & mask_loorder)
//     + ... == sizeof...(Bits), where all Bits are unique,
//    then saturation mask is (carry << 1) - (((carry >> (Bits0 - 1)) | (carry >> (Bits1 - 1)) | ...) & mask_loorder)
// 3. ...

template<class Integer, Integer hiorder, Integer loorder, class UniqueBitsSeq>
struct is_saturation_mask_of_type_1_helper;
template<class Integer, Integer hiorder, Integer loorder, size_t ...Bits>
struct is_saturation_mask_of_type_1_helper<Integer, hiorder, loorder, integer_seq<Bits...>> {
    static const size_t value = sum<
        bit_count<(hiorder >> (Bits - 1)) & loorder>::value...
    >::value;
};

template<class Integer, size_t ...Bits>
struct is_saturation_mask_of_type_1 {
    static const Integer hiorder = mask_hiorder<Integer, Bits...>::value;
    static const Integer loorder = mask_loorder<Integer, Bits...>::value;
    using unique_bits = unique<integer_seq<Bits...>>;

    static const bool value = sizeof...(Bits) == is_saturation_mask_of_type_1_helper<
        Integer, hiorder, loorder, unique_bits>::value;
};

template<class Integer, size_t ...Bits>
struct detect_saturation_mask_type_impl {
    using type = typename std::conditional<
        all_same<integer_seq<Bits...>>::value,
        size_t_<0>,
        typename std::conditional<
            is_saturation_mask_of_type_1<Integer, Bits...>::value,
            size_t_<1>,
            size_t_<2>
        >::type
    >::type;
};
template<class Integer, size_t ...Bits>
using detect_saturation_mask_type = typename detect_saturation_mask_type_impl<Integer, Bits...>::type;

// Type 0 saturation mask
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer dispatch_make_unsigned_saturation_mask(
    Integer carrys, size_t_<0> /* all bits are the same */)
{
    return (carrys >> (Bits0 - 1));
}

// Type 1 saturation mask
#if __cpp_fold_expressions
template<size_t ...Bits, class Integer>
constexpr Integer make_unsigned_saturation_mask_type_1(Integer carrys, integer_seq<Bits...>) {
    return static_cast<Integer>((... | (carrys >> (Bits - 1))));
}
#else
template<class Integer>
constexpr Integer make_unsigned_saturation_mask_type_1(Integer carrys, seq<>) {
    return 0;
}
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer make_unsigned_saturation_mask_type_1(Integer carrys, integer_seq<Bits0, Bits...>) {
    return static_cast<Integer>((carrys >> (Bits0 - 1))
        | make_unsigned_saturation_mask_type_1(carrys, integer_seq<Bits...>()));
}
#endif
template<size_t ...Bits, class Integer>
constexpr Integer dispatch_make_unsigned_saturation_mask(
    Integer carrys, size_t_<1> /* packs of variable length (type 1) */)
{
    using loorder = mask_loorder<Integer, Bits...>;
    return make_unsigned_saturation_mask_type_1(carrys, unique<integer_seq<Bits...>>())
        & loorder::value;
}

// Type 2 saturation mask
template<class Integer, class MasksMap>
struct unsigned_saturation_mask_type_2_helper;

template<class Integer, class ...Keys, class ...Offsets>
struct unsigned_saturation_mask_type_2_helper<Integer, seq<seq<Keys, Offsets>...>> {
    using type = seq<
        seq<Keys, std::integral_constant<Integer, make_mask_from_bitpos_impl<Integer, Offsets>::value>>...
    >;
};

template<class Integer, size_t ...Bits>
struct unsigned_saturation_mask_type_2_impl {
    // Offsets of all masks
    using offsets = mask_offsets_vector<Bits...>;
    // Zip it with mask sizes
    using masks_and_sizes = zip<integer_seq<Bits...>, offsets>;
    // Make map of masks <MaskSize, <Offset0, Offset1, ...>>
    using masks_map = unzip_to_map<masks_and_sizes>;
    // Make map of masks <MaskSize, LoOrder mask>
    using type = typename unsigned_saturation_mask_type_2_helper<Integer, masks_map>::type;
};
template<class Integer, size_t ...Bits>
using unsigned_saturation_mask_type_2 = typename unsigned_saturation_mask_type_2_impl<Integer, Bits...>::type;

#if __cpp_fold_expressions
template<class ...Masks, class Integer>
constexpr Integer make_unsigned_saturation_mask_type_2(Integer carrys, seq<Masks...>) {
    // Masks = seq<MaskSize, LoOrder Mask for MaskSize>
    return (... |
        ((carrys >> (take_1st<Masks>::value - 1)) & take_2nd<Masks>::value)
    );
}
#else
template<class Integer>
constexpr Integer make_unsigned_saturation_mask_type_2(Integer /*carrys*/, seq<>) { return 0; }
template<class Mask, class ...Masks, class Integer>
constexpr Integer make_unsigned_saturation_mask_type_2(Integer carrys, seq<Mask, Masks...>) {
    // Masks = seq<MaskSize, LoOrder Mask for MaskSize>
    return static_cast<Integer>(((carrys >> (take_1st<Mask>::value - 1)) & take_2nd<Mask>::value) |
        make_unsigned_saturation_mask_type_2(carrys, seq<Masks...>()));
}
#endif

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer dispatch_make_unsigned_saturation_mask(
    Integer carrys, size_t_<2> /* packs of variable length (type 2) */)
{
    return make_unsigned_saturation_mask_type_2(carrys,
        unsigned_saturation_mask_type_2<Integer, Bits0, Bits...>());
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer make_unsigned_saturation_mask(Integer carrys)
{
    return static_cast<Integer>((carrys << 1) -
        dispatch_make_unsigned_saturation_mask<Bits0, Bits...>(
            carrys, detect_saturation_mask_type<Integer, Bits0, Bits...>())
    );
}

// Unsigned sum with saturation
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_unsigned_saturate(Integer sum, Integer carrys)
{
    return sum | make_unsigned_saturation_mask<Bits0, Bits...>(carrys);
}

/////////////////////////////////////////////////////////////////////
// Signed sum with saturation

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer make_signed_saturation_mask(Integer overflow) {
    using saturation_mask_type = detect_saturation_mask_type<Integer, Bits0, Bits...>;
    return overflow - dispatch_make_unsigned_saturation_mask<Bits0, Bits...>(overflow, saturation_mask_type());
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer apply_signed_saturation(Integer sum, Integer overflow)
{
    return ((sum ^ overflow) | make_signed_saturation_mask<Bits0, Bits...>(overflow)) ^
        make_signed_saturation_mask<Bits0, Bits...>(overflow & ~sum);
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_signed_saturate(Integer a, Integer b, Integer sum)
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return apply_signed_saturation<Bits0, Bits...>(
        sum, static_cast<Integer>((~(a ^ b)) & (sum ^ b) & mask2::value));
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer sub_signed_saturate(Integer a, Integer b, Integer diff)
{
    using mask2 = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return apply_signed_saturation<Bits0, Bits...>(
        diff, static_cast<Integer>(overflow_signed_sub_vector(a, b, diff) & mask2::value));
}

template<class Integer, size_t Bits0, size_t ...Bits>
constexpr Integer make_truncate(Integer value0,
    typename std::integral_constant<Integer, Bits>::value_type ...values)
{
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integral won't fit given number of bits");

    using offset_and_mask = detail::offset_and_mask_vector<Bits0, Bits...>;
    return detail::make_truncated_int<Integer>(offset_and_mask(), value0, values...);
}

template<class Integer>
constexpr Integer interleave(Integer a, Integer b, Integer mask) {
    return (a & mask) | (b & ~mask);
}

template<size_t RequiredBits> struct find_appropriate_int;
template<> struct find_appropriate_int<8> { using type = uint8_t; };
template<> struct find_appropriate_int<16> { using type = uint16_t; };
template<> struct find_appropriate_int<32> { using type = uint32_t; };
template<> struct find_appropriate_int<64> { using type = uint64_t; };

// Make packed_int from vector of bits
template<class Integer, class BitsVector> struct packed_int_from_seq_impl;
template<class Integer, size_t Bits0, size_t ...Bits>
struct packed_int_from_seq_impl<Integer, integer_seq<Bits0, Bits...>> {
    using type = packed_int<Integer, Bits0, Bits...>;
};
template<class Integer, class BitsVector>
using packed_int_from_seq = typename packed_int_from_seq_impl<Integer, BitsVector>::type;

// Sliced seq
template<size_t First, size_t Last, class Integer, size_t Bits0, size_t ...Bits>
struct sliced_int_impl {
    static_assert(First < Last && Last <= sizeof...(Bits) + 1, "Incorrect slice bounds");

    using sliced_bits_seq = slice<First, Last, integer_seq<Bits0, Bits...>>;
    using type = packed_int_from_seq<Integer, sliced_bits_seq>;
};
template<size_t First, size_t Last, class Integer, size_t Bits0, size_t ...Bits>
using sliced_int = typename sliced_int_impl<First, Last, Integer, Bits0, Bits...>::type;

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

    bool operator==(packed_int other) const { return m_value == other.m_value; }
    bool operator!=(packed_int other) const { return m_value != other.m_value; }

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

///////////////////////////////////////////////////////////////////////////////

template<size_t Index, size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer get(packed_int<Integer, Bits0, Bits...> packed_int)
{
    static_assert(Index <= sizeof...(Bits), "Incorrect index");
    using mask_and_offset = detail::take_offset_and_mask<Index, Bits0, Bits...>;

    return (packed_int.value() >> detail::take_1st<mask_and_offset>::value)
        & detail::all_ones<Integer, detail::take_2nd<mask_and_offset>::value>::value;
}

template<size_t Index, size_t Bits0, size_t ...Bits, class Integer>
constexpr typename std::make_signed<Integer>::type
    get_signed(packed_int<Integer, Bits0, Bits...> packed_int)
{
    static_assert(Index <= sizeof...(Bits), "Incorrect index");
    // <offset, mask>
    using mask_and_offset = detail::take_offset_and_mask<Index, Bits0, Bits...>;
    using hi_order_bit_no = detail::size_t_<
        detail::take_2nd<mask_and_offset>::value + detail::take_1st<mask_and_offset>::value>;

    return static_cast<typename std::make_signed<Integer>::type>(
        packed_int.value() << (sizeof(Integer) * 8 - hi_order_bit_no::value)) >>
        (sizeof(Integer) * 8 - detail::take_2nd<mask_and_offset>::value);
}

///////////////////////////////////////////////////////////////////////////////

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
            // potentially overflown result
            add_wrap(a, b).value(),
            // carry vector
            static_cast<Integer>(detail::carry_add_vector(a.value(), b.value()) & mask2::value)
        )
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
        packed(
            detail::add_unsigned_saturate<Bits0, Bits...>(
                // potentially overflown result
                add_wrap(a, packed(~b.value())).value(),
                // overflow vector
                static_cast<Integer>(detail::carry_sub_vector(a.value(), b.value()) & mask2::value)
            )
        ),
        packed(mask3::value)
    );
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

///////////////////////////////////////////////////////////////////////////////

template<size_t Start, size_t End, size_t Bits0, size_t ...Bits, class Integer>
constexpr detail::sliced_int<Start,End,Integer,Bits0,Bits...>
    slice(packed_int<Integer, Bits0, Bits...> value) noexcept
{
    using lo_bits_sum = detail::sum_seq<detail::take_front_n<Start, detail::integer_seq<Bits0, Bits...>>>;
    using middle_bits_sum = detail::sum_seq<detail::slice<Start, End, detail::integer_seq<Bits0, Bits...>>>;

    return detail::sliced_int<Start,End,Integer,Bits0,Bits...>(
        (value.value() >> lo_bits_sum::value) & detail::all_ones<Integer, middle_bits_sum::value>::value);
}

///////////////////////////////////////////////////////////////////////////////

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> min_unsigned(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using hi_order_bits_mask = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return packed_int<Integer, Bits0, Bits...>(
        detail::interleave(a.value(), b.value(),
            static_cast<Integer>(
                detail::make_unsigned_saturation_mask<Bits0, Bits...>(
                    detail::carry_sub_vector(a.value(), b.value()) & hi_order_bits_mask::value)
            )
        )
    );
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> max_unsigned(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using hi_order_bits_mask = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return packed_int<Integer, Bits0, Bits...>(
        detail::interleave(a.value(), b.value(),
            static_cast<Integer>(
                detail::make_unsigned_saturation_mask<Bits0, Bits...>(
                    detail::carry_sub_vector(b.value(), a.value()) & hi_order_bits_mask::value)
            )
        )
    );
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> min_signed(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using hi_order_bits_mask = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return packed_int<Integer, Bits0, Bits...>(
        detail::interleave(a.value(), b.value(),
            static_cast<Integer>(
                detail::make_unsigned_saturation_mask<Bits0, Bits...>(
                    detail::carry_sub_vector(
                        a.value() ^ hi_order_bits_mask::value,
                        b.value() ^ hi_order_bits_mask::value
                    ) & hi_order_bits_mask::value)
            )
        )
    );
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> max_signed(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b) noexcept
{
    using hi_order_bits_mask = detail::mask_hiorder<Integer, Bits0, Bits...>;
    return packed_int<Integer, Bits0, Bits...>(
        detail::interleave(a.value(), b.value(),
            static_cast<Integer>(
                detail::make_unsigned_saturation_mask<Bits0, Bits...>(
                    detail::carry_sub_vector(
                        b.value() ^ hi_order_bits_mask::value,
                        a.value() ^ hi_order_bits_mask::value
                    ) & hi_order_bits_mask::value)
            )
        )
    );
}

} // namespace pint
