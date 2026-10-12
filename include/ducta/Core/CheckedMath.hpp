/**
 * @file CheckedMath.hpp
 * @brief Integer arithmetic that detects overflow, in portable C++17
 *        (no compiler intrinsics such as __builtin_mul_overflow)
 */

#ifndef DUCTA_CORE_CHECKED_MATH_HPP
#define DUCTA_CORE_CHECKED_MATH_HPP

#include "ducta/Core/Limits.hpp"
#include "ducta/Core/Types/Integers.hpp"

namespace ducta {

/*
 * @brief out = a + b
 * @return false (out unchanged) if the sum does not fit into int64_t
 */
constexpr bool checked_add(int64_t a, int64_t b, int64_t& out) noexcept
{
    constexpr int64_t max = ::ducta::numeric_limits<int64_t>::max();
    constexpr int64_t min = ::ducta::numeric_limits<int64_t>::min();
    if ((b > 0 && a > max - b) || (b < 0 && a < min - b))
    {
        return false;
    }
    out = a + b;
    return true;
}

/*
 * @brief out = a * b
 * @return false (out unchanged) if the product does not fit into int64_t
 */
constexpr bool checked_mul(int64_t a, int64_t b, int64_t& out) noexcept
{
    constexpr int64_t max = ::ducta::numeric_limits<int64_t>::max();
    constexpr int64_t min = ::ducta::numeric_limits<int64_t>::min();
    // Every bound below is computed by a division that cannot overflow itself
    if (a > 0)
    {
        if (b > 0 ? a > max / b : b < min / a)
        {
            return false;
        }
    }
    else if (a < 0)
    {
        if (b > 0 ? a < min / b : (b != 0 && b < max / a))
        {
            return false;
        }
    }
    out = a * b;
    return true;
}

} // namespace ducta

#endif // DUCTA_CORE_CHECKED_MATH_HPP
