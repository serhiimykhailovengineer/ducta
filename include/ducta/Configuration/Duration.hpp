/**
 * @file Duration.hpp
 * @brief Decoding of chrono durations (e.g. Chrono::TimestampUS) from a configuration
 *
 * A duration is written as a string of one or more <number><unit> parts:
 *     "10ms", "1.5s", "250 us", "1h30m", "-5ms"
 * Units: ns, us (or µs), ms, s, m (or min), h, d. The number may have a fraction; the result
 * must be a whole number of the destination's ticks ("1.5us" does not fit microseconds).
 * A plain integer is a count of the destination's ticks (e.g. microseconds for TimestampUS).
 */

#ifndef DUCTA_CONFIGURATION_DURATION_HPP
#define DUCTA_CONFIGURATION_DURATION_HPP

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Chrono/Chrono.hpp"

#include "ducta/Core/Limits.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"

namespace ducta {
namespace Configuration {

namespace Private {

enum class DurationError : uint8_t
{
    None,
    Syntax,
    Unit,
    Range,
    Precision
};

struct DurationUnit
{
    StringView name;
    int64_t nanoseconds;
};

inline bool lookup_duration_unit(StringView name, int64_t& nanoseconds) noexcept
{
    static constexpr DurationUnit units[] = {
        {"ns", 1},
        {"us", 1000},
        {"\xC2\xB5s", 1000}, // µs
        {"ms", 1000000},
        {"s", 1000000000},
        {"m", 60LL * 1000000000},
        {"min", 60LL * 1000000000},
        {"h", 3600LL * 1000000000},
        {"d", 86400LL * 1000000000},
    };
    for (DurationUnit const& unit : units)
    {
        if (unit.name == name)
        {
            nanoseconds = unit.nanoseconds;
            return true;
        }
    }
    return false;
}

inline bool is_digit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

inline bool is_space(char c) noexcept
{
    return c == ' ' || c == '\t';
}

/* Unit characters: ASCII letters and the bytes of 'µ' (UTF-8 C2 B5) */
inline bool is_unit_char(char c) noexcept
{
    unsigned char const u = static_cast<unsigned char>(c);
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || u == 0xC2 || u == 0xB5;
}

/*
 * Parse text as a sum of <number><unit> parts into nanoseconds.
 * unit receives the offending unit for DurationError::Unit.
 */
inline DurationError parse_duration_ns(StringView text, int64_t& out, StringView& unit) noexcept
{
    size_t i = 0;
    size_t const n = text.size();
    auto skip_spaces = [&] {
        while (i < n && is_space(text[i]))
        {
            ++i;
        }
    };

    skip_spaces();
    bool negative = false;
    if (i < n && (text[i] == '+' || text[i] == '-'))
    {
        negative = text[i] == '-';
        ++i;
    }

    int64_t total = 0;
    bool any = false;
    while (true)
    {
        skip_spaces();
        if (i == n)
        {
            break;
        }

        // <integer>[.<fraction>]
        int64_t integer = 0;
        int64_t fraction = 0;
        int64_t fraction_scale = 1;
        bool digits = false;
        while (i < n && is_digit(text[i]))
        {
            if (__builtin_mul_overflow(integer, 10, &integer) ||
                __builtin_add_overflow(integer, text[i] - '0', &integer))
            {
                return DurationError::Range;
            }
            digits = true;
            ++i;
        }
        if (i < n && text[i] == '.')
        {
            ++i;
            while (i < n && is_digit(text[i]))
            {
                if (fraction_scale > ::ducta::numeric_limits<int64_t>::max() / 10)
                {
                    return DurationError::Precision;
                }
                fraction = fraction * 10 + (text[i] - '0');
                fraction_scale *= 10;
                digits = true;
                ++i;
            }
        }
        if (!digits)
        {
            return DurationError::Syntax;
        }

        // <unit>
        skip_spaces();
        size_t const unit_begin = i;
        while (i < n && is_unit_char(text[i]))
        {
            ++i;
        }
        unit = text.substr(unit_begin, i - unit_begin);
        if (unit.empty())
        {
            return DurationError::Syntax;
        }
        int64_t unit_ns = 0;
        if (!lookup_duration_unit(unit, unit_ns))
        {
            return DurationError::Unit;
        }

        // integer * unit + fraction * unit / scale, exactly
        int64_t part = 0;
        int64_t fraction_ns = 0;
        if (__builtin_mul_overflow(integer, unit_ns, &part) ||
            __builtin_mul_overflow(fraction, unit_ns, &fraction_ns))
        {
            return DurationError::Range;
        }
        if (fraction_ns % fraction_scale != 0)
        {
            return DurationError::Precision;
        }
        if (__builtin_add_overflow(part, fraction_ns / fraction_scale, &part) ||
            __builtin_add_overflow(total, part, &total))
        {
            return DurationError::Range;
        }
        any = true;
    }

    if (!any)
    {
        return DurationError::Syntax;
    }
    out = negative ? -total : total;
    return DurationError::None;
}

constexpr int64_t gcd(int64_t a, int64_t b) noexcept
{
    while (b != 0)
    {
        int64_t const r = a % b;
        a = b;
        b = r;
    }
    return a;
}

/*
 * Convert nanoseconds to ticks of Period (num / den seconds), exactly:
 * ticks = nanoseconds * den / (num * 1e9), which must be a whole number
 */
template <class Period>
DurationError nanoseconds_to_ticks(int64_t nanoseconds, int64_t& ticks) noexcept
{
    constexpr int64_t num = static_cast<int64_t>(Period::num);
    constexpr int64_t den = static_cast<int64_t>(Period::den);
    int64_t tick_ns = 0;
    if (__builtin_mul_overflow(num, int64_t{1000000000}, &tick_ns))
    {
        return DurationError::Range;
    }
    // Reduce den / tick_ns: ticks = (nanoseconds / divisor) * multiplier
    int64_t const common = gcd(den, tick_ns);
    int64_t const multiplier = den / common;
    int64_t const divisor = tick_ns / common;
    if (nanoseconds % divisor != 0)
    {
        return DurationError::Precision;
    }
    if (__builtin_mul_overflow(nanoseconds / divisor, multiplier, &ticks))
    {
        return DurationError::Range;
    }
    return DurationError::None;
}

template <class Period>
constexpr StringView tick_name() noexcept
{
    if constexpr (Period::num == 1 && Period::den == 1000000000)
    {
        return "1ns";
    }
    else if constexpr (Period::num == 1 && Period::den == 1000000)
    {
        return "1us";
    }
    else if constexpr (Period::num == 1 && Period::den == 1000)
    {
        return "1ms";
    }
    else if constexpr (Period::num == 1 && Period::den == 1)
    {
        return "1s";
    }
    else
    {
        return "the duration's tick";
    }
}

} // namespace Private

template <class Rep, class Period>
struct ValueDecoder<Chrono::chrono::duration<Rep, Period>>
{
    using Duration = Chrono::chrono::duration<Rep, Period>;

    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::String || kind == NodeKind::Integer;
    }

    static bool decode(PropNode node, Duration& out, Diagnostics& diagnostics)
    {
        int64_t ticks = 0;
        if (node.kind() == NodeKind::Integer)
        {
            ticks = node.as_integer();
        }
        else if (node.kind() == NodeKind::String)
        {
            StringView const text = node.as_string();
            StringView unit;
            int64_t nanoseconds = 0;
            Private::DurationError error = Private::parse_duration_ns(text, nanoseconds, unit);
            if (error == Private::DurationError::None)
            {
                error = Private::nanoseconds_to_ticks<Period>(nanoseconds, ticks);
            }
            switch (error)
            {
            case Private::DurationError::None:
                break;
            case Private::DurationError::Syntax:
                diagnostics.errorf(
                    node, "invalid duration '{}' (expected e.g. \"10ms\", \"1.5s\", \"1h30m\")", text);
                return false;
            case Private::DurationError::Unit:
                diagnostics.errorf(
                    node, "unknown duration unit '{}' (expected ns, us, ms, s, m, h, d)", unit);
                return false;
            case Private::DurationError::Range:
                diagnostics.errorf(node, "duration '{}' is out of range", text);
                return false;
            case Private::DurationError::Precision:
                diagnostics.errorf(
                    node,
                    "duration '{}' is not a whole number of {}",
                    text,
                    Private::tick_name<Period>());
                return false;
            }
        }
        else
        {
            diagnostics.errorf(
                node,
                "expected a duration string (e.g. \"10ms\") or integer, got {}",
                to_string(node.kind()));
            return false;
        }

        if (ticks < static_cast<int64_t>(::ducta::numeric_limits<Rep>::min()) ||
            ticks > static_cast<int64_t>(::ducta::numeric_limits<Rep>::max()))
        {
            diagnostics.errorf(node, "duration is out of range");
            return false;
        }
        out = Duration{static_cast<Rep>(ticks)};
        return true;
    }
};

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_DURATION_HPP
