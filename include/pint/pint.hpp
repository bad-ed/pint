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

// Make mask which is equal to (1 << (Bits0 - 1)) | (1 << (Bits1 - 1)) | (1 << (Bits2 - 1)) | ...
template<class T, class IntegerSeq> struct mask_for_add_impl;

#if __cpp_fold_expressions
template<class T, size_t ...Bits>
struct mask_for_add_impl<T, integer_seq<Bits...>> {
    static const T value = (... | (1 << (Bits - 1)));
};
#else
template<class T, size_t Bits0, size_t ...Bits>
struct mask_for_add_impl<T, integer_seq<Bits0, Bits...>> {
    static const T value = (1 << (Bits0 - 1)) |
        mask_for_add_impl<T, integer_seq<Bits...>>::value;
};
template<class T> struct mask_for_add_impl<T, integer_seq<>> {
    static const T value = 0;
};
#endif

template<class T, size_t ...Bits>
using mask_for_add = std::integral_constant<T,
    mask_for_add_impl<T,
        make_sum_vector<Bits...>
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
using mask_offsets_vector = prepend_seq<
    size_t_<0>,
    make_sum_vector_n<sizeof...(Bits) - 1, Bits...> // (0, Bits0, Bits0 + Bits1, Bits0 + Bits1 + Bits2, ...)
>;
template<size_t... Bits>
using offset_and_mask_vector = zip<
    mask_offsets_vector<Bits...>,
    integer_seq<Bits...>
>;
template<class Integer, class OffsetAndMask>
using shifted_mask = std::integral_constant<Integer, (
    all_ones<Integer, take_2nd<OffsetAndMask>::value>::value <<
        take_1st<OffsetAndMask>::value
)>;

#if __cpp_fold_expressions
template<class Integer, class ...Values, class ...MasksAndOffsets>
constexpr Integer make_truncated_int(seq<MasksAndOffsets...>, Values ...values)
{
    return (... | ((values & all_ones<Integer, take_2nd<MasksAndOffsets>::value>::value) << take_1st<MasksAndOffsets>::value));
}
#else
template<class Integer> constexpr Integer make_truncated_int(seq<>) { return 0; }
template<class Integer, class ...Values, class ...MasksAndOffsets>
constexpr Integer make_truncated_int(seq<MasksAndOffsets...>, Integer value0, Values ...values)
{
    using my_mask_and_offset = take_1st<seq<MasksAndOffsets...>>;

    return ((value0 & all_ones<Integer, take_2nd<my_mask_and_offset>::value>::value) <<
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
        (carrys & shifted_mask<Integer, OffsetAndMasks>::value ? shifted_mask<Integer, OffsetAndMasks>::value : 0));
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

    return (carrys & shifted_mask<Integer, offset_and_mask>::value ? shifted_mask<Integer, offset_and_mask>::value : 0) |
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
constexpr Integer add_signed_saturate_same_length2(Integer sum, Integer overflow)
{
    return sum ^ signed_payload_bits_from_overflow<Bits0>(sum & overflow);
}

template<size_t Bits0, class Integer>
constexpr Integer add_signed_saturate_same_length(Integer sum, Integer overflow)
{
    return add_signed_saturate_same_length2<Bits0>(static_cast<Integer>(
        (sum ^ overflow) | signed_payload_bits_from_overflow<Bits0>(overflow)), overflow);
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_signed_saturate(
    Integer a, Integer b, Integer sum, std::true_type /* packs of same length */)
{
    using mask2 = detail::mask_for_add<Integer, Bits0, Bits...>;
    return add_signed_saturate_same_length<Bits0>(sum, static_cast<Integer>((~(a ^ b)) & (sum ^ b) & mask2::value));
}

} // namespace detail

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_wrap(Integer a, Integer b) {
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integral won't fit given number of bits");

    using mask2 = detail::mask_for_add<Integer, Bits0, Bits...>;
    using mask1 = std::integral_constant<Integer, (~mask2::value)
        & detail::all_ones<Integer, detail::sum<Bits0, Bits...>::value>::value>;

    return ((a & mask1::value) + (b & mask1::value)) ^
        ((a ^ b) & mask2::value);
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_unsigned_saturate(Integer a, Integer b) {
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integral won't fit given number of bits");

    using mask2 = detail::mask_for_add<Integer, Bits0, Bits...>;

    return detail::add_unsigned_saturate<Bits0, Bits...>(
        add_wrap<Bits0, Bits...>(a, b), // potentially overflown result
        static_cast<Integer>(((a & b) | ((a | b) & ~(a + b))) & mask2::value), // carry vector
        detail::all_same<detail::integer_seq<Bits0, Bits...>>());
}

template<size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer add_signed_saturate(Integer a, Integer b) {
    static_assert(sizeof(Integer) * 8 >= detail::sum<Bits0, Bits...>::value,
        "Integral won't fit given number of bits");

    return detail::add_signed_saturate<Bits0, Bits...>(
        a, b,
        add_wrap<Bits0, Bits...>(a, b),
        detail::all_same<detail::integer_seq<Bits0, Bits...>>());
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

} // namespace pint