#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "pint/pint.hpp"

using TestVector = std::vector<std::pair<uint32_t, uint32_t>>;

TestVector GetRandomPairs(size_t amount_of_pairs) {
    std::vector<std::pair<uint32_t, uint32_t>> result;
    result.reserve(amount_of_pairs);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;

    for (; amount_of_pairs; --amount_of_pairs) {
        result.emplace_back(dist(gen), dist(gen));
    }

    return result;
}

class Bench {
public:
    ~Bench() {
        auto duration = std::chrono::steady_clock::now() - m_started;

        std::cout << "Time taken: " << std::chrono::duration<double, std::milli>(duration).count() << "ms\n";
    }

private:
    const std::chrono::steady_clock::time_point m_started{std::chrono::steady_clock::now()};
};

////////////////////////////////////////////////////////////////////////////////

uint32_t Baseline(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pair.first + pair.second;

    return sum;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t AddWrapUsingPint(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pint::add_wrap<1,2,3,4,5,6,11>(pair.first, pair.second);

    return sum;
}

uint32_t AddWrapUsingBitshifting(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers) {
        auto a = pair.first;
        auto b = pair.second;

        const uint32_t result = (((a & 1) + (b & 1)) & 1) |
            (((a & 6) + (b & 6)) & 6) |
            (((a & 0x38) + (b & 0x38)) & 0x38) |
            (((a & 0x3C0) + (b & 0x3C0)) & 0x3C0) |
            (((a & 0x7C00) + (b & 0x7C00)) & 0x7C00) |
            (((a & 0x1F8000) + (b & 0x1F8000)) & 0x1F8000) |
            ((a & 0xFFE00000) + (b & 0xFFE00000));

        sum += result;
    }

    return sum;
}

uint32_t AddWrapUsingUnion(const TestVector &numbers) {
    union SumUnion {
        struct {
            uint32_t a: 1;
            uint32_t b: 2;
            uint32_t c: 3;
            uint32_t d: 4;
            uint32_t e: 5;
            uint32_t f: 6;
            uint32_t g: 11;
        };

        uint32_t value;
    };

    uint32_t sum = 0;
    for (auto &pair : numbers) {
        SumUnion a,b,c;

        a.value = pair.first;
        b.value = pair.second;

        c.a = a.a + b.a;
        c.b = a.b + b.b;
        c.c = a.c + b.c;
        c.d = a.d + b.d;
        c.e = a.e + b.e;
        c.f = a.f + b.f;
        c.g = a.g + b.g;

        sum += c.value;
    }

    return sum;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t SubWrapUsingPint(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pint::sub_wrap<1,2,3,4,5,6,11>(pair.first, pair.second);

    return sum;
}

uint32_t SubWrapUsingUnion(const TestVector &numbers) {
    union SumUnion {
        struct {
            uint32_t a: 1;
            uint32_t b: 2;
            uint32_t c: 3;
            uint32_t d: 4;
            uint32_t e: 5;
            uint32_t f: 6;
            uint32_t g: 11;
        };

        uint32_t value;
    };

    uint32_t sum = 0;
    for (auto &pair : numbers) {
        SumUnion a,b,c;

        a.value = pair.first;
        b.value = pair.second;

        c.a = a.a - b.a;
        c.b = a.b - b.b;
        c.c = a.c - b.c;
        c.d = a.d - b.d;
        c.e = a.e - b.e;
        c.f = a.f - b.f;
        c.g = a.g - b.g;

        sum += c.value;
    }

    return sum;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t AddUnsignedSaturationUsingPint(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pint::add_unsigned_saturate<1,2,3,4,5,6,11>(pair.first, pair.second);

    return sum;
}

uint32_t AddUnsignedSaturationUsingUnion(const TestVector &numbers) {
    union SumUnion {
        struct {
            uint32_t a: 1;
            uint32_t b: 2;
            uint32_t c: 3;
            uint32_t d: 4;
            uint32_t e: 5;
            uint32_t f: 6;
            uint32_t g: 11;
        };

        uint32_t value;
    };

    uint32_t sum = 0;
    for (auto &pair : numbers) {
        SumUnion a,b,c;

        a.value = pair.first;
        b.value = pair.second;

        c.a = a.a + b.a;
        if (c.a < a.a && c.a < b.a) c.a = 1;

        c.b = a.b + b.b;
        if (c.b < a.b && c.b < b.b) c.b = 3;

        c.c = a.c + b.c;
        if (c.c < a.c && c.c < b.c) c.c = 7;

        c.d = a.d + b.d;
        if (c.d < a.d && c.d < b.d) c.d = 15;

        c.e = a.e + b.e;
        if (c.e < a.e && c.e < b.e) c.e = 31;

        c.f = a.f + b.f;
        if (c.f < a.f && c.f < b.f) c.f = 63;

        c.g = a.g + b.g;
        if (c.g < a.g && c.g < b.g) c.g = 2047;

        sum += c.value;
    }

    return sum;
}

////////////////////////////////////////////////////////////////////////////////

template<size_t bits>
int32_t clamp(int32_t value) {
    static const int32_t kMinv = static_cast<uint32_t>(~0) << (bits - 1);
    static const int32_t kMaxv = (1 << (bits - 1)) - 1;

    if (value < kMinv) return kMinv;
    if (value > kMaxv) return kMaxv;
    return value;
}

uint32_t AddSignedSaturationUsingPint(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pint::add_signed_saturate<1,2,3,4,5,6,11>(pair.first, pair.second);

    return sum;
}

uint32_t AddSignedSaturationUsingUnion(const TestVector &numbers) {
    union SumUnion {
        struct {
            int32_t a: 1;
            int32_t b: 2;
            int32_t c: 3;
            int32_t d: 4;
            int32_t e: 5;
            int32_t f: 6;
            int32_t g: 11;
        };

        uint32_t value;
    };

    uint32_t sum = 0;
    for (auto &pair : numbers) {
        SumUnion a,b,c;

        a.value = pair.first;
        b.value = pair.second;

        c.a = clamp<1>(a.a + b.a);
        c.b = clamp<2>(a.b + b.b);
        c.c = clamp<3>(a.c + b.c);
        c.d = clamp<4>(a.d + b.d);
        c.e = clamp<5>(a.e + b.e);
        c.f = clamp<6>(a.f + b.f);
        c.g = clamp<11>(a.g + b.g);

        sum += c.value;
    }

    return sum;
}

////////////////////////////////////////////////////////////////////////////////

uint32_t AddSignedSaturationUsingPint2(const TestVector &numbers) {
    uint32_t sum = 0;
    for (auto &pair : numbers)
        sum += pint::add_signed_saturate<4,4,4,4,4,4,4,4>(pair.first, pair.second);

    return sum;
}

uint32_t AddSignedSaturationUsingUnion2(const TestVector &numbers) {
    union SumUnion {
        struct {
            int32_t a: 4;
            int32_t b: 4;
            int32_t c: 4;
            int32_t d: 4;
            int32_t e: 4;
            int32_t f: 4;
            int32_t g: 4;
            int32_t h: 4;
        };

        uint32_t value;
    };

    uint32_t sum = 0;
    for (auto &pair : numbers) {
        SumUnion a,b,c;

        a.value = pair.first;
        b.value = pair.second;

        c.a = clamp<4>(a.a + b.a);
        c.b = clamp<4>(a.b + b.b);
        c.c = clamp<4>(a.c + b.c);
        c.d = clamp<4>(a.d + b.d);
        c.e = clamp<4>(a.e + b.e);
        c.f = clamp<4>(a.f + b.f);
        c.g = clamp<4>(a.g + b.g);
        c.h = clamp<4>(a.h + b.h);

        sum += c.value;
    }

    return sum;
}

///////////////////////////////////////////////////////////////////////////////

int main() {
    struct {
        uint32_t (* volatile bench_func)(const TestVector &);
        const char *bench_descr;
    } static const kTests[] = {
        { Baseline,                "warmup            " },
        { Baseline,                "baseline          " },
        { AddWrapUsingPint,        "pint    |add|wrap " },
        { AddWrapUsingUnion,       "union   |add|wrap " },

        { AddUnsignedSaturationUsingPint, "pint    |add|sat/u" },
        { AddUnsignedSaturationUsingUnion,"union   |add|sat/u" },

        { AddSignedSaturationUsingPint, "pint    |add|sat/s" },
        { AddSignedSaturationUsingUnion,"union   |add|sat/s" },

        { AddSignedSaturationUsingPint2, "pint/eq |add|sat/s" },
        { AddSignedSaturationUsingUnion2,"union/eq|add|sat/s" },

        { SubWrapUsingPint,        "pint    |sub|wrap " },
        { SubWrapUsingUnion,       "union   |sub|wrap " },
    };

    std::cout << "Generating random pairs\n";
    auto random_pairs = GetRandomPairs(100'000'000);

    for (auto &bench_info : kTests)
    {
        std::cout << bench_info.bench_descr << " = ";

        Bench $;
        std::cout << bench_info.bench_func(random_pairs) << " # ";
    }
}
