# 🍺 Pint

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/0a179e1d68c343bb9d9d4d12756f5b8e)](https://app.codacy.com/app/bad-ed/pint?utm_source=github.com&utm_medium=referral&utm_content=bad-ed/pint&utm_campaign=Badge_Grade_Settings)

Header-only library for packed integers manipulations. Library provides a set of functions for adding/subtracting/etc... of packed integers in high-performance manner (most of functions are branch-free).

## Requirements

A C++ compiler that supports C++11.

## Usage example

```cpp
#include <pint/pint.hpp>

// MyPack is integer which consists of 3 integers
// of lengths 5, 6 and 5 bits respectively
using MyPack = pint::make_packed_int<5,6,5>;

// Create first packed integer.
// a == 2 | (10 << 6) | (20 << 11)
constexpr auto a = MyPack(2, 10, 20);
// Second packed integer
constexpr auto b = MyPack(1, 2, 12);

// Calculate sum of these packed integers
constexpr auto sum = pint::add_wrap(a, b);
static_assert(sum == MyPack(3, 12, 0), "WTF?!");

// Calculate sum with saturation
constexpr auto sum_saturated = pint::add_unsigned_saturate(a, b);
static_assert(sum_saturated == MyPack(3, 12, 31), "WTF?!");
```

## Reference

### Classes and helpers

#### packed_int

`packed_int` is a class which represents pack of integers

```cpp
template<class Integer, size_t Bits0, size_t ...Bits>
class packed_int {
public:
    // Construct class from single value which represents all contained packs
    constexpr explicit packed_int(Integer value);

    // Construct class from individual values of each pack.
    // Constructor consists of `sizeof...(Bits)+1` Integer arguments
    // NOTE: This constructor enabled only if `sizeof...(Bits) != 0`
    constexpr packed_int(Integer value1, ..., Integer value_n);

    // Returns stored packs of integers as single integer value
    constexpr Integer value() const;

    // Comparison operators
    constexpr bool operator==(packed_int other) const;
    constexpr bool operator!=(packed_int other) const;

    // Bitwise operators
    constexpr packed_int operator|(packed_int other) const;
    constexpr packed_int operator&(packed_int other) const;
    constexpr packed_int operator^(packed_int other) const;
};
```

`Integer` type argument defines the type used for storing value. Only unsigned integers are allowed. Passing signed integer type raises compilation error.

If `Integer` can't fit all the provided packs (`sizeof(Integer) < (Bits0 + ... + Bits)`), error message is emitted.

For example `packed_int<uint8_t,1,2,6>` is not permitted, because `1+2+6=9` bits cannot be represented by `uint8_t`.

#### make_packed_int

```cpp
template<size_t Bits0, size_t ...Bits>
using make_packed_int = packed_int<𝑖𝑚𝑝𝑙𝑒𝑚𝑒𝑛𝑡𝑎𝑡𝑖𝑜𝑛-𝑑𝑒𝑓𝑖𝑛𝑒𝑑, Bits0, Bits...>;
```

`make_packed_int` is helper alias for defining `packed_int`. It automatically detects type of integer of minimum size, which is capable to hold all the bit packs.

For example `make_packed_int<1,7>` renders to `packed_int<uint8_t,1,7>`, whereas `make_packed_int<2,7>` renders to `packed_int<uint16_t,2,7>`.

### Generic functions

#### get

```cpp
template<size_t Index, size_t Bits0, size_t ...Bits, class Integer>
constexpr Integer get(packed_int<Integer, Bits0, Bits...> packed_int);
```

Get the value of pack at index `Index`.

**Examples**

```cpp
using MyPack = make_packed_int<3,5>;
get<0>(MyPack(4, 23)); // returns 4
get<1>(MyPack(4, 23)); // returns 23
```

#### get_signed

```cpp
template<size_t Index, size_t Bits0, size_t ...Bits, class Integer>
constexpr typename std::make_signed<Integer>::type
    get_signed(packed_int<Integer, Bits0, Bits...> packed_int);
```

Get sign extended value of pack at index `Index`.

**Examples**

```cpp
using MyPack = make_packed_int<4,6>;
get_signed<0>(MyPack(-4, 23)); // returns -4
get_signed<1>(MyPack(-4, 23)); // returns 23

// Compared to `get` function
get<0>(MyPack(-4, 23)); // returns 12
```

#### slice

```cpp
template<size_t Start, size_t End, size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, 𝑏𝑖𝑡𝑠-𝑠𝑙𝑖𝑐𝑒(Start,End,{Bits0,Bits...})>
    slice(packed_int<Integer, Bits0, Bits...> value);
```

Get the slice of `packed_int`. Returns value which consists of a portion of given `value`, portion is defined by `Start` and `End` parameters.

**Examples**

```cpp
using MyPack = make_packed_int<2,3,4,5,6>;

// a has type `packed_int<MyPack::value_type, 4, 5>`
constexpr auto a = slice<2,4>(MyPack(3,7,15,31,63));
get<0>(a); // == 15
get<1>(a); // == 31
```

### Arithmetic functions

#### add_wrap

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_wrap(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Add two integer packs. In case if sum wraps around zero, carry bit is thrown away.

**Examples**

```cpp
using MyPack = make_packed_int<5,6,5>;

constexpr auto a = MyPack(2, 10, 20);
constexpr auto b = MyPack(1, 58, 12);

add_wrap(a, b); // == MyPack(3, 4, 0);
```

#### add_unsigned_saturate

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_unsigned_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Add two integer packs. In case if sum wraps around zero, that sum is saturated.

**Examples**

```cpp
using MyPack = make_packed_int<5,6,5>;

constexpr auto a = MyPack(2, 10, 20);
constexpr auto b = MyPack(1, 58, 12);

add_unsigned_saturate(a, b); // == MyPack(3, 63, 31);
```

#### add_signed_saturate

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> add_signed_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Add two integer packs. Each pack is interpreted as signed integer.

If two positive values are added and their sum overflows max positive value, that sum is replaced by max positive value.

If two positive values are added and their sum overflows min negative value, that sum is replaced by min negative value.

**Examples**

```cpp
using MyPack = make_packed_int<5,6,5>;

constexpr auto a = MyPack(2, 10, -10);
constexpr auto b = MyPack(1, 28, -12);

add_signed_saturate(a, b); // == MyPack(3, 31, -16);
```

#### sub_wrap

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_wrap(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Subtracts second pack from the first pack.

**Examples**

```cpp
using MyPack = make_packed_int<4, 4, 4>;
sub_wrap(MyPack(1, 4, 2), MyPack(7, 2, 6)); // == MyPack(10, 2, 12)
```

#### sub_unsigned_saturate

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_unsigned_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Subtracts second pack from the first pack. If difference overflows zero, difference is replaced by zero.

**Examples**

```cpp
using MyPack = make_packed_int<4, 4, 4>;
sub_unsigned_saturate(MyPack(1, 4, 2), MyPack(7, 2, 6)); // == MyPack(0, 2, 0)
```

#### sub_signed_saturate

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> sub_signed_saturate(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Subtracts second pack from the first pack.

If first pack is positive and second pack is negative and their difference overflows max positive value, difference is replaced by max positive value.

If first pack is negative and second pack is positive and their difference overflows min negative value, difference is replaced by min negative value.

**Examples**

```cpp
using MyPack = make_packed_int<4, 6, 4>;

constexpr auto a = MyPack(4, -3, 7);
constexpr auto b = MyPack(-6, 30, 1);

sub_signed_saturate(a, b); // == MyPack(7, -32, 6)
```

### Min / Max

#### min_unsigned

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> min_unsigned(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Interpret all packs as unsigned, find min in each pack.

**Examples**

```cpp
using MyPack = make_packed_int<4,6,4>;

constexpr auto a = MyPack(4,5,3);
constexpr auto b = MyPack(1,15,3);

min_unsigned(a, b); // == MyPack(1,5,3)
```

#### max_unsigned

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> max_unsigned(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Interpret all packs as unsigned, find max in each pack.

**Examples**

```cpp
using MyPack = make_packed_int<4,6,4>;

constexpr auto a = MyPack(4,5,3);
constexpr auto b = MyPack(1,15,3);

max_unsigned(a, b); // == MyPack(4,15,3)
```

#### min_signed

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> min_signed(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Interpret all packs as signed, find min in each pack.

**Examples**

```cpp
using MyPack = make_packed_int<4,6,4>;

constexpr auto a = MyPack(-1,5,0);
constexpr auto b = MyPack(4,-2,7);

min_signed(a, b); // == MyPack(-1,-2,0)
```

#### max_signed

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> max_signed(
    packed_int<Integer, Bits0, Bits...> a,
    packed_int<Integer, Bits0, Bits...> b);
```

Interpret all packs as signed, find max in each pack.

**Examples**

```cpp
using MyPack = make_packed_int<4,6,4>;

constexpr auto a = MyPack(-1,5,0);
constexpr auto b = MyPack(4,-2,7);

max_signed(a, b); // == MyPack(4,5,7)
```

### Shifting

#### shift_left

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> shift_left(
    packed_int<Integer, Bits0, Bits...> value,
    size_t amount);
```

Shift all packs to the left by the `amount` of bits.

**Examples**

```cpp
using MyPack = pint::make_packed_int<3,7,6>;

constexpr auto value = MyPack(1,2,3);
shift_left(value, 3); // MyPack(0,16,24);
```

#### shift_right_unsigned

```cpp
template<size_t Bits0, size_t ...Bits, class Integer>
constexpr packed_int<Integer, Bits0, Bits...> shift_right_unsigned(
    packed_int<Integer, Bits0, Bits...> value,
    size_t amount);
```

Unsigned right shift of all packs by the `amount` of bits.

**Examples**

```cpp
using MyPack = pint::make_packed_int<3,7,6>;

constexpr auto value = MyPack(5,106,42);
shift_right_unsigned(value, 4); // MyPack(0,6,2);
```

## Credits

The idea to create library sparkled after reading article [A Proposal for Hardware-Assisted Arithmetic Overflow Detection for Array and Bitfield Operations](http://www.emulators.com/docs/LazyOverflowDetect_Final.pdf)
