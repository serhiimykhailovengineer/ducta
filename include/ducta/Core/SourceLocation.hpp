#ifndef DUCTA_CORE_SOURCE_LOCATION_HPP
#define DUCTA_CORE_SOURCE_LOCATION_HPP

// With the standard backend in C++20, current() is implemented with std::source_location
// (and a std::source_location converts implicitly); otherwise (C++17 or the embedded
// backend) with compiler builtins.
#if !defined(DUCTA_USE_EMBEDDED_BACKEND) && __cplusplus >= 202002L && defined(__has_include)
#if __has_include(<source_location>)
#include <source_location>
#if defined(__cpp_lib_source_location)
#define DUCTA_HAS_STD_SOURCE_LOCATION 1
#endif
#endif
#endif

#include "ducta/Core/Types/Integers.hpp"

#if defined(__has_builtin)
#define DUCTA_HAS_BUILTIN(x) __has_builtin(x)
#else
#define DUCTA_HAS_BUILTIN(x) 0
#endif

namespace ducta {

/*
 * @brief Location in a source: C++ code (SourceLocation::current()) or a loaded file
 *        (json / yaml / xml), with the same interface as std::source_location.
 * @note Line and column are 1-based; 0 means unknown. Members that do not apply are empty,
 *       e.g. function_name() of a value loaded from a file, or file_name() of a value
 *       loaded from a stream. Names are not owned: they must outlive the location.
 */
class SourceLocation
{
public:
    constexpr SourceLocation() noexcept = default;

    constexpr SourceLocation(
        uint_least32_t line,
        uint_least32_t column,
        char const* file = "",
        char const* function = "") noexcept
        : m_file{file != nullptr ? file : ""}
        , m_function{function != nullptr ? function : ""}
        , m_line{line}
        , m_column{column}
    {
    }

#if defined(DUCTA_HAS_STD_SOURCE_LOCATION)
    constexpr SourceLocation(std::source_location const& location) noexcept
        : SourceLocation(location.line(), location.column(), location.file_name(), location.function_name())
    {
    }

    static constexpr SourceLocation current(
        std::source_location location = std::source_location::current()) noexcept
    {
        return SourceLocation{location};
    }
#else
    static constexpr SourceLocation current(
#if DUCTA_HAS_BUILTIN(__builtin_FILE) || defined(__GNUC__)
        char const* file = __builtin_FILE(),
        char const* function = __builtin_FUNCTION(),
        uint_least32_t line = __builtin_LINE(),
#else
        char const* file = "",
        char const* function = "",
        uint_least32_t line = 0,
#endif
#if DUCTA_HAS_BUILTIN(__builtin_COLUMN)
        uint_least32_t column = __builtin_COLUMN()
#else
        uint_least32_t column = 0
#endif
            ) noexcept
    {
        return SourceLocation{line, column, file, function};
    }
#endif

    constexpr uint_least32_t line() const noexcept
    {
        return m_line;
    }

    /* 0 when unknown (e.g. the compiler cannot provide it) */
    constexpr uint_least32_t column() const noexcept
    {
        return m_column;
    }

    constexpr char const* file_name() const noexcept
    {
        return m_file;
    }

    constexpr char const* function_name() const noexcept
    {
        return m_function;
    }

    constexpr bool valid() const noexcept
    {
        return m_line != 0;
    }

    friend constexpr bool operator==(SourceLocation const& a, SourceLocation const& b) noexcept
    {
        return a.m_line == b.m_line && a.m_column == b.m_column && equal(a.m_file, b.m_file) &&
               equal(a.m_function, b.m_function);
    }

    friend constexpr bool operator!=(SourceLocation const& a, SourceLocation const& b) noexcept
    {
        return !(a == b);
    }

private:
    static constexpr bool equal(char const* a, char const* b) noexcept
    {
        for (; *a != '\0' && *a == *b; ++a, ++b)
        {
        }
        return *a == *b;
    }

private:
    char const* m_file = "";
    char const* m_function = "";
    uint_least32_t m_line = 0;
    uint_least32_t m_column = 0;
};

} // namespace ducta

#endif // DUCTA_CORE_SOURCE_LOCATION_HPP
