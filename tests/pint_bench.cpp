#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#if defined(__x86_64__) || defined(_M_X64)
#define PINT_HAVE_SSE
#endif

#ifdef PINT_HAVE_SSE
#include <emmintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

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

template<size_t bits>
uint32_t uclamp(uint32_t value) {
    static const uint32_t kMaxv = (1 << bits) - 1;

    if (value > kMaxv) return kMaxv;
    return value;
}

class Bench {
public:
    std::chrono::steady_clock::duration TimeSinceStart() const {
        return std::chrono::steady_clock::now() - m_started;
    }

private:
    const std::chrono::steady_clock::time_point m_started{std::chrono::steady_clock::now()};
};

////////////////////////////////////////////////////////////////////////////////

class PairsBenchmarks : public benchmark::Fixture {
public:
    void SetUp(benchmark::State &st) override {
        sum = 0;
    }

    void TearDown(benchmark::State &state) override {
        state.SetItemsProcessed(numbers.size() * state.iterations());
        state.SetLabel("Sum = " + std::to_string(sum));
    }

protected:
    static TestVector numbers;
    uint32_t sum;
};

TestVector PairsBenchmarks::numbers = GetRandomPairs(100000000);

BENCHMARK_F(PairsBenchmarks, Baseline)(benchmark::State& state) {
    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pair.first + pair.second;
    }
}

using AddWrap = PairsBenchmarks;

BENCHMARK_F(AddWrap, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,1,2,3,4,5,6,11>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_wrap(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddWrap, BitshiftingNaive)(benchmark::State& state) {
    for (auto $ : state) {
        sum = 0;
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
    }
}

BENCHMARK_F(AddWrap, Union)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
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
    }
}

////////////////////////////////////////////////////////////////////////////////

using AddWrap0 = PairsBenchmarks;

BENCHMARK_F(AddWrap0, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,8,8,8,8>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_wrap(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

#ifdef PINT_HAVE_SSE
BENCHMARK_F(AddWrap0, SSE2)(benchmark::State& state) {
    for (auto $ : state) {
        auto tmp_sum = _mm_setzero_si128();
        const __m128i *data = reinterpret_cast<const __m128i *>(numbers.data());

        for (size_t i = 0; i < numbers.size(); i += 4, data += 2) {
#if 1
            //auto a = _mm_setr_epi32(numbers[i].first, numbers[i].second, numbers[i+1].first, numbers[i+1].second);
            //auto b = _mm_setr_epi32(numbers[i+2].first, numbers[i+2].second, numbers[i+3].first, numbers[i+3].second);
            auto a = _mm_loadu_si128(data);
            auto b = _mm_loadu_si128(data + 1);

            auto a0 = _mm_unpacklo_epi32(a, b); // {[0].first, [2].first, [0].second, [2].second}
            auto b0 = _mm_unpackhi_epi32(a, b); // {[1].first, [3].first, [1].second, [3].second}

            a = _mm_unpacklo_epi32(a0, b0);
            b = _mm_unpackhi_epi32(a0, b0);
#else
            auto a = _mm_set_epi32(numbers[i].first, numbers[i+1].first, numbers[i+2].first, numbers[i+3].first);
            auto b = _mm_set_epi32(numbers[i].second, numbers[i+1].second, numbers[i+2].second, numbers[i+3].second);
#endif
            auto c = _mm_add_epi8(a, b);
            tmp_sum = _mm_add_epi32(c, tmp_sum);
        }

        tmp_sum = _mm_add_epi32(tmp_sum, _mm_srli_si128(tmp_sum, 8));
        tmp_sum = _mm_add_epi32(tmp_sum, _mm_srli_si128(tmp_sum, 4));
        sum = _mm_cvtsi128_si32(tmp_sum);
    }
}
#endif

#ifdef __ARM_NEON
BENCHMARK_F(AddWrap0, Neon)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,8,8,8,8>;

    for (auto $ : state) {
        sum = 0;

        uint32x4_t tmp_sum = vdupq_n_u32(0);

        const uint32_t *data = reinterpret_cast<const uint32_t *>(numbers.data());
        for (size_t i = 0; i < numbers.size(); i += 4, data += 8) {
            auto pairs = vld2q_u32(data);
            auto res = vaddq_u8(pairs.val[0], pairs.val[1]);
            tmp_sum = vaddq_u32(res, tmp_sum);
        }

        auto r1 = vadd_u32(vget_low_u32(tmp_sum), vget_high_u32(tmp_sum));
        sum = vget_lane_u32(vpadd_u32(r1, r1), 0);
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////

using SubWrap = PairsBenchmarks;

BENCHMARK_F(SubWrap, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,1,2,3,4,5,6,11>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::sub_wrap(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(SubWrap, Union)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
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
    }
}

////////////////////////////////////////////////////////////////////////////////

using AddSatU2 = PairsBenchmarks;

BENCHMARK_F(AddSatU2, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,1,2,3,4,5,6,11>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_unsigned_saturate(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddSatU2, Union)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
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
    }
}

BENCHMARK_F(AddSatU2, UnionClamp)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers) {
            SumUnion a,b,c;

            a.value = pair.first;
            b.value = pair.second;

            c.a = uclamp<1>(a.a + b.a);
            c.b = uclamp<2>(a.b + b.b);
            c.c = uclamp<3>(a.c + b.c);
            c.d = uclamp<4>(a.d + b.d);
            c.e = uclamp<5>(a.e + b.e);
            c.f = uclamp<6>(a.f + b.f);
            c.g = uclamp<11>(a.g + b.g);

            sum += c.value;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

using AddSatU1 = PairsBenchmarks;

BENCHMARK_F(AddSatU1, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,1,3,5,11>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_unsigned_saturate(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddSatU1, Union)(benchmark::State& state) {
    union SumUnion {
        struct {
            uint32_t a: 1;
            uint32_t c: 3;
            uint32_t e: 5;
            uint32_t g: 11;
        };

        uint32_t value;
    };

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers) {
            SumUnion a,b,c;

            a.value = pair.first;
            b.value = pair.second;
            c.value = 0;

            c.a = a.a + b.a;
            if (c.a < a.a && c.a < b.a) c.a = 1;

            c.c = a.c + b.c;
            if (c.c < a.c && c.c < b.c) c.c = 7;

            c.e = a.e + b.e;
            if (c.e < a.e && c.e < b.e) c.e = 31;

            c.g = a.g + b.g;
            if (c.g < a.g && c.g < b.g) c.g = 2047;

            sum += c.value;
        }
    }
}

BENCHMARK_F(AddSatU1, UnionClamp)(benchmark::State& state) {
    union SumUnion {
        struct {
            uint32_t a: 1;
            uint32_t c: 3;
            uint32_t e: 5;
            uint32_t g: 11;
        };

        uint32_t value;
    };

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers) {
            SumUnion a,b,c;

            a.value = pair.first;
            b.value = pair.second;
            c.value = 0;

            c.a = uclamp<1>(a.a + b.a);
            c.c = uclamp<3>(a.c + b.c);
            c.e = uclamp<5>(a.e + b.e);
            c.g = uclamp<11>(a.g + b.g);

            sum += c.value;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

using AddSatU0 = PairsBenchmarks;

BENCHMARK_F(AddSatU0, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,8,8,8,8>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_unsigned_saturate(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddSatU0, Pint64)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint64_t,8,8,8,8,8,8,8,8>;

    for (auto $ : state) {
        using PackedSum = pint::packed_int<uint64_t, 32, 32>;
        PackedSum tmp_sum{0};

        for (size_t i = 0; i < numbers.size(); i += 2) {
            auto a = PackedInt((static_cast<uint64_t>(numbers[i+1].first) << 32) | numbers[i].first);
            auto b = PackedInt((static_cast<uint64_t>(numbers[i+1].second) << 32) | numbers[i].second);

            auto c = PackedSum(pint::add_unsigned_saturate(a, b).value());
            tmp_sum = pint::add_wrap(tmp_sum, c);
        }

        sum = static_cast<uint32_t>(tmp_sum.value() >> 32) + static_cast<uint32_t>(tmp_sum.value());
    }
}

#ifdef PINT_HAVE_SSE
BENCHMARK_F(AddSatU0, SSE2)(benchmark::State& state) {
    for (auto $ : state) {
        auto tmp_sum = _mm_setzero_si128();
        const __m128i *data = reinterpret_cast<const __m128i *>(numbers.data());

        for (size_t i = 0; i < numbers.size(); i += 4, data += 2) {
#if 1
            //auto a = _mm_setr_epi32(numbers[i].first, numbers[i].second, numbers[i+1].first, numbers[i+1].second);
            //auto b = _mm_setr_epi32(numbers[i+2].first, numbers[i+2].second, numbers[i+3].first, numbers[i+3].second);
            auto a = _mm_loadu_si128(data);
            auto b = _mm_loadu_si128(data + 1);

            auto a0 = _mm_unpacklo_epi32(a, b); // {[0].first, [2].first, [0].second, [2].second}
            auto b0 = _mm_unpackhi_epi32(a, b); // {[1].first, [3].first, [1].second, [3].second}

            a = _mm_unpacklo_epi32(a0, b0);
            b = _mm_unpackhi_epi32(a0, b0);
#else
            auto a = _mm_set_epi32(numbers[i].first, numbers[i+1].first, numbers[i+2].first, numbers[i+3].first);
            auto b = _mm_set_epi32(numbers[i].second, numbers[i+1].second, numbers[i+2].second, numbers[i+3].second);
#endif
            auto c = _mm_adds_epu8(a, b);
            tmp_sum = _mm_add_epi32(c, tmp_sum);
        }

        tmp_sum = _mm_add_epi32(tmp_sum, _mm_srli_si128(tmp_sum, 8));
        tmp_sum = _mm_add_epi32(tmp_sum, _mm_srli_si128(tmp_sum, 4));
        sum = _mm_cvtsi128_si32(tmp_sum);
    }
}
#endif

#ifdef __ARM_NEON
BENCHMARK_F(AddSatU0, Neon)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,8,8,8,8>;

    for (auto $ : state) {
        sum = 0;

        uint32x4_t tmp_sum = vdupq_n_u32(0);

        const uint32_t *data = reinterpret_cast<const uint32_t *>(numbers.data());
        for (size_t i = 0; i < numbers.size(); i += 4, data += 8) {
            auto pairs = vld2q_u32(data);
            auto res = vqaddq_u8(pairs.val[0], pairs.val[1]);
            tmp_sum = vaddq_u32(res, tmp_sum);
        }

        auto r1 = vadd_u32(vget_low_u32(tmp_sum), vget_high_u32(tmp_sum));
        sum = vget_lane_u32(vpadd_u32(r1, r1), 0);
    }
}
#endif

BENCHMARK_F(AddSatU0, UnionClamp)(benchmark::State& state) {
    union SumUnion {
        struct {
            uint32_t a: 8;
            uint32_t b: 8;
            uint32_t c: 8;
            uint32_t d: 8;
        };

        uint32_t value;
    };

    for (auto $ : state) {
        sum = 0;

        for (auto &pair : numbers) {
            SumUnion a,b,c;

            a.value = pair.first;
            b.value = pair.second;
            c.value = 0;

            c.a = uclamp<8>(a.a + b.a);
            c.b = uclamp<8>(a.b + b.b);
            c.c = uclamp<8>(a.c + b.c);
            c.d = uclamp<8>(a.d + b.d);

            sum += c.value;
        }
    }
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

using AddSatS2 = PairsBenchmarks;

BENCHMARK_F(AddSatS2, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,1,2,3,4,5,6,11>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_signed_saturate(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddSatS2, UnionClamp)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
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
    }
}

////////////////////////////////////////////////////////////////////////////////

using AddSatS0 = PairsBenchmarks;

BENCHMARK_F(AddSatS0, Pint)(benchmark::State& state) {
    using PackedInt = pint::packed_int<uint32_t,4,4,4,4,4,4,4,4>;

    for (auto $ : state) {
        sum = 0;
        for (auto &pair : numbers)
            sum += pint::add_signed_saturate(PackedInt(pair.first), PackedInt(pair.second)).value();
    }
}

BENCHMARK_F(AddSatS0, UnionClamp)(benchmark::State& state) {
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

    for (auto $ : state) {
        sum = 0;
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
    }
}
