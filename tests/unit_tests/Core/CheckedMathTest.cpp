#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "ducta/Core/CheckedMath.hpp"

using namespace ducta;

namespace {

constexpr int64_t kMax = std::numeric_limits<int64_t>::max();
constexpr int64_t kMin = std::numeric_limits<int64_t>::min();

/* Values around every boundary that matters for 64-bit addition and multiplication */
std::vector<int64_t> boundaryValues()
{
    std::vector<int64_t> values{0, 1, -1, 2, -2, 3, -3, 10, -10, 1000000000, -1000000000};
    for (int64_t base :
         {kMax,
          kMin,
          kMax / 2,
          kMin / 2,
          int64_t{3037000499},
          int64_t{-3037000499},
          int64_t{4294967296},
          int64_t{-4294967296}})
    {
        for (int64_t delta = -2; delta <= 2; ++delta)
        {
            int64_t value = 0;
            if (checked_add(base, delta, value))
            {
                values.push_back(value);
            }
        }
    }
    return values;
}

} // namespace

TEST(CheckedMathTest, add_examples)
{
    int64_t out = 7;
    EXPECT_TRUE(checked_add(2, 3, out));
    EXPECT_EQ(out, 5);
    EXPECT_TRUE(checked_add(kMax, kMin, out));
    EXPECT_EQ(out, -1);

    out = 7;
    EXPECT_FALSE(checked_add(kMax, 1, out));
    EXPECT_FALSE(checked_add(kMin, -1, out));
    EXPECT_EQ(out, 7); // unchanged on overflow
}

TEST(CheckedMathTest, mul_examples)
{
    int64_t out = 7;
    EXPECT_TRUE(checked_mul(-4, 5, out));
    EXPECT_EQ(out, -20);
    EXPECT_TRUE(checked_mul(kMin, 1, out));
    EXPECT_EQ(out, kMin);
    EXPECT_TRUE(checked_mul(0, kMin, out));
    EXPECT_EQ(out, 0);

    out = 7;
    EXPECT_FALSE(checked_mul(kMin, -1, out));
    EXPECT_FALSE(checked_mul(-1, kMin, out));
    EXPECT_FALSE(checked_mul(int64_t{3037000500}, int64_t{3037000500}, out));
    EXPECT_FALSE(checked_mul(int64_t{-3037000500}, int64_t{3037000500}, out));
    EXPECT_EQ(out, 7); // unchanged on overflow
}

TEST(CheckedMathTest, constexpr_usable)
{
    constexpr bool ok = [] {
        int64_t out = 0;
        return checked_mul(1000, 1000, out) && out == 1000000;
    }();
    static_assert(ok, "checked_mul must be usable in constant expressions");
}

#if defined(__SIZEOF_INT128__)
// The test (not the library) uses 128-bit integers as the exact reference
TEST(CheckedMathTest, matches_exact_arithmetic_at_the_boundaries)
{
    auto const values = boundaryValues();
    for (int64_t a : values)
    {
        for (int64_t b : values)
        {
            __int128 const sum = static_cast<__int128>(a) + b;
            __int128 const product = static_cast<__int128>(a) * b;
            bool const sum_fits = sum >= kMin && sum <= kMax;
            bool const product_fits = product >= kMin && product <= kMax;

            int64_t out = 0;
            ASSERT_EQ(checked_add(a, b, out), sum_fits) << a << " + " << b;
            if (sum_fits)
            {
                ASSERT_EQ(out, static_cast<int64_t>(sum)) << a << " + " << b;
            }
            ASSERT_EQ(checked_mul(a, b, out), product_fits) << a << " * " << b;
            if (product_fits)
            {
                ASSERT_EQ(out, static_cast<int64_t>(product)) << a << " * " << b;
            }
        }
    }
}
#endif
