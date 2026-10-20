#ifndef DUCTA_CONFIGURATION_PRIVATE_JSON_INL
#define DUCTA_CONFIGURATION_PRIVATE_JSON_INL

#include "ducta/Configuration/Json.hpp"

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Config.hpp"
#include "ducta/Core/Limits.hpp"
#include "ducta/Core/SourceLocation.hpp"
#include "ducta/Core/Types/Array.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Utility.hpp"

// jsmn is used as a header-only tokenizer: static functions, strict JSON
#if !defined(JSMN_H)
#define JSMN_STATIC
#define JSMN_STRICT
#include <jsmn.h>
#undef JSMN_STATIC
#undef JSMN_STRICT
#endif

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#endif

namespace ducta {
namespace Configuration {
namespace Private {

/* Converts byte offsets into 1-based line / column (columns count UTF-8 characters) */
class TextCursor
{
public:
    explicit TextCursor(StringView text) noexcept
        : m_text{text}
    {
    }

    SourceLocation location(size_t offset, char const* file) noexcept
    {
        if (offset < m_offset)
        {
            m_offset = 0;
            m_line = 1;
            m_column = 1;
        }
        for (; m_offset < offset && m_offset < m_text.size(); ++m_offset)
        {
            unsigned char const c = static_cast<unsigned char>(m_text[m_offset]);
            if (c == '\n')
            {
                ++m_line;
                m_column = 1;
            }
            else if ((c & 0xC0) != 0x80)
            {
                ++m_column;
            }
        }
        return SourceLocation{m_line, m_column, file};
    }

private:
    StringView m_text;
    size_t m_offset{0};
    uint_least32_t m_line{1};
    uint_least32_t m_column{1};
};

class JsonLoader
{
public:
    JsonLoader(StringView text, PropTree& tree, Diagnostics& diagnostics, JsonOptions options) noexcept
        : m_text{text}
        , m_tree{tree}
        , m_diagnostics{diagnostics}
        , m_options{options}
        , m_cursor{text}
    {
    }

    bool load();

private:
    using Token = jsmntok_t;

    static bool is_space(char c) noexcept
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    /* Strings tokens exclude their quotes */
    static size_t start_of(Token const& token) noexcept
    {
        return static_cast<size_t>(token.type == JSMN_STRING ? token.start - 1 : token.start);
    }

    static size_t end_of(Token const& token) noexcept
    {
        return static_cast<size_t>(token.type == JSMN_STRING ? token.end + 1 : token.end);
    }

    SourceLocation location(size_t offset) noexcept
    {
        return m_cursor.location(offset, m_file);
    }

    template <class... Args>
    void report(SourceLocation location, Path path, format_string<Args...> fmt, Args&&... args)
    {
        Diagnostic diagnostic;
        diagnostic.location = location;
        diagnostic.path = ::ducta::move(path);
        format_to(diagnostic.message, fmt, ::ducta::forward<Args>(args)...);
        m_diagnostics.add(::ducta::move(diagnostic));
        m_ok = false;
    }

    /* Syntax errors stop loading */
    template <class... Args>
    void syntax_error(size_t offset, format_string<Args...> fmt, Args&&... args)
    {
        report(location(offset), Path{}, fmt, ::ducta::forward<Args>(args)...);
        m_syntax_ok = false;
    }

    void report_jsmn_error(int error, size_t offset);
    void report_unexpected(size_t offset);
    bool expect_gap(size_t from, size_t to, char separator, char closing = '\0');

    Path child_path(NodeId parent, StringView key, size_t position) const;
    NodeId added(Expected<NodeId, Error> result, SourceLocation location, Path const& path);

    size_t value(size_t index, NodeId parent, StringView key, size_t position, size_t depth);
    size_t members(size_t index, NodeId object, size_t depth);
    size_t elements(size_t index, NodeId array, size_t depth);
    void primitive(Token const& token, NodeId parent, StringView key, Path const& path);

    template <class StringType>
    bool decode_string(Token const& token, StringType& out, Path const& path);

private:
    StringView m_text;
    PropTree& m_tree;
    Diagnostics& m_diagnostics;
    JsonOptions m_options;
    TextCursor m_cursor;
    char const* m_file{""};
    bool m_ok{true};
    bool m_syntax_ok{true};

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    Array<Token, DUCTA_CONFIG_MAX_JSON_TOKENS> m_tokens{};
#else
    std::vector<Token> m_tokens;
#endif
    size_t m_count{0};
};

inline bool is_hex(char c) noexcept
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline uint32_t hex_value(StringView digits) noexcept
{
    uint32_t value = 0;
    for (char c : digits)
    {
        value <<= 4;
        if (c >= '0' && c <= '9')
        {
            value |= static_cast<uint32_t>(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            value |= static_cast<uint32_t>(c - 'a' + 10);
        }
        else
        {
            value |= static_cast<uint32_t>(c - 'A' + 10);
        }
    }
    return value;
}

/* JSON number grammar: -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)? */
inline bool is_json_number(StringView text, bool& is_integer) noexcept
{
    size_t i = 0;
    size_t const n = text.size();
    auto digit = [&](size_t at) {
        return at < n && text[at] >= '0' && text[at] <= '9';
    };

    if (i < n && text[i] == '-')
    {
        ++i;
    }
    if (!digit(i))
    {
        return false;
    }
    if (text[i] == '0')
    {
        ++i;
    }
    else
    {
        while (digit(i))
        {
            ++i;
        }
    }

    is_integer = true;
    if (i < n && text[i] == '.')
    {
        ++i;
        if (!digit(i))
        {
            return false;
        }
        while (digit(i))
        {
            ++i;
        }
        is_integer = false;
    }
    if (i < n && (text[i] == 'e' || text[i] == 'E'))
    {
        ++i;
        if (i < n && (text[i] == '+' || text[i] == '-'))
        {
            ++i;
        }
        if (!digit(i))
        {
            return false;
        }
        while (digit(i))
        {
            ++i;
        }
        is_integer = false;
    }
    return i == n;
}

/* Parses a valid JSON integer; false if it does not fit into int64_t */
inline bool parse_integer(StringView text, int64_t& out) noexcept
{
    bool const negative = !text.empty() && text[0] == '-';
    uint64_t const limit = negative
                               ? static_cast<uint64_t>(::ducta::numeric_limits<int64_t>::max()) + 1
                               : static_cast<uint64_t>(::ducta::numeric_limits<int64_t>::max());
    uint64_t magnitude = 0;
    for (size_t i = negative ? 1 : 0; i < text.size(); ++i)
    {
        uint64_t const digit = static_cast<uint64_t>(text[i] - '0');
        if (magnitude > (limit - digit) / 10)
        {
            return false;
        }
        magnitude = magnitude * 10 + digit;
    }
    if (negative)
    {
        out = magnitude == limit ? ::ducta::numeric_limits<int64_t>::min()
                                 : -static_cast<int64_t>(magnitude);
    }
    else
    {
        out = static_cast<int64_t>(magnitude);
    }
    return true;
}

/*
 * Whether significand * 10^(leading_exponent - 18) rounds to infinity, for a significand of
 * 19 digits (the first one non-zero). A double overflows from 2^1024 - 2^970 (DBL_MAX plus half
 * a unit in the last place; the tie rounds to the even infinity) = 1.797693134862315807937...e308.
 * Digits after the 19th are not kept: with equal first 19 digits they would have to reach
 * 937... to overflow, so such a number is taken as finite.
 */
constexpr bool overflows_double(uint64_t significand, int64_t leading_exponent) noexcept
{
    constexpr uint64_t threshold = 1797693134862315807ULL; // first 19 digits, followed by 937...
    if (leading_exponent != 308)
    {
        return leading_exponent > 308;
    }
    return significand > threshold;
}

/*
 * Parses a valid JSON number with a fraction or an exponent without the C library (strtod pulls
 * in malloc, locales and stdio: ~20 KB of flash on a microcontroller).
 *
 * Exact (correctly rounded, the same as strtod) when the significant digits fit into 2^53 and
 * the decimal exponent is within +-22, which covers configuration values such as 0.1, 2.5,
 * 1e-3 or 123.456: the integer and the power of ten are both exact doubles, so one
 * multiplication or division rounds once. Otherwise returns false, and the result is the
 * approximation within 3 units in the last place, 0 on underflow or an infinity on overflow.
 * Overflow itself is decided exactly (see overflows_double), so the largest finite values such
 * as 1.7976931348623157e308 stay finite.
 */
inline bool parse_float(StringView text, double& out) noexcept
{
    static constexpr double powers_of_ten[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                                               1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                                               1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
    constexpr int max_exact_power = 22;
    constexpr uint64_t max_exact_integer = uint64_t{1} << 53;
    constexpr int max_digits = 19; // fits into uint64_t

    size_t i = 0;
    size_t const n = text.size();
    bool const negative = n > 0 && text[0] == '-';
    if (negative)
    {
        ++i;
    }

    // Up to max_digits significant digits; the next ones only move the decimal point
    uint64_t mantissa = 0;
    int digits = 0;
    bool truncated = false;
    int64_t exponent = 0;
    auto is_digit = [&](size_t at) {
        return at < n && text[at] >= '0' && text[at] <= '9';
    };
    for (; is_digit(i); ++i)
    {
        if (digits < max_digits)
        {
            mantissa = mantissa * 10 + static_cast<uint64_t>(text[i] - '0');
            digits += mantissa != 0 ? 1 : 0;
        }
        else
        {
            truncated = truncated || text[i] != '0';
            ++exponent;
        }
    }
    if (i < n && text[i] == '.')
    {
        for (++i; is_digit(i); ++i)
        {
            if (digits < max_digits)
            {
                mantissa = mantissa * 10 + static_cast<uint64_t>(text[i] - '0');
                digits += mantissa != 0 ? 1 : 0;
                --exponent;
            }
            else
            {
                truncated = truncated || text[i] != '0';
            }
        }
    }
    if (i < n && (text[i] == 'e' || text[i] == 'E'))
    {
        ++i;
        bool const negative_exponent = i < n && text[i] == '-';
        if (i < n && (text[i] == '+' || text[i] == '-'))
        {
            ++i;
        }
        int64_t written = 0;
        for (; is_digit(i); ++i)
        {
            if (written < 100000) // far beyond the range of double: the result is 0 or infinity
            {
                written = written * 10 + (text[i] - '0');
            }
        }
        exponent += negative_exponent ? -written : written;
    }

    double value = static_cast<double>(mantissa);
    bool exact = !truncated && mantissa <= max_exact_integer && exponent >= -max_exact_power &&
                 exponent <= max_exact_power;
    if (mantissa == 0 || exponent == 0)
    {
        // value is final
    }
    else if (exact)
    {
        value = exponent > 0 ? value * powers_of_ten[exponent] : value / powers_of_ten[-exponent];
    }
    else
    {
        /* 10^exponent = 10^remainder * 10^(23 * steps): at most three roundings, plus the one of
         * the constant 10^(23 * steps) */
        static constexpr double large_powers[] = {
            1e0, 1e23, 1e46, 1e69, 1e92, 1e115, 1e138, 1e161, 1e184, 1e207, 1e230, 1e253, 1e276, 1e299};
        static constexpr double small_powers[] = {
            1e0, 1e-23, 1e-46, 1e-69, 1e-92, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299};
        constexpr int64_t step = 23;
        constexpr int64_t max_steps = 13;
        constexpr int64_t max_scale = max_exact_power + step * max_steps; // 321

        int64_t scale = exponent > 0 ? exponent : -exponent;
        for (; scale > max_scale; scale -= max_exact_power) // only reached by 0 or infinity
        {
            value = exponent > 0 ? value * powers_of_ten[max_exact_power]
                                 : value / powers_of_ten[max_exact_power];
        }
        int64_t const steps = scale / step < max_steps ? scale / step : max_steps;
        int64_t const remainder = scale - steps * step;
        if (exponent > 0)
        {
            value = value * powers_of_ten[remainder] * large_powers[steps];
            /* The roundings above may cross the overflow boundary either way: decide it exactly
             * from the decimal digits (at most 3 units in the last place below DBL_MAX) */
            uint64_t significand = mantissa;
            for (int i = digits; i < max_digits; ++i)
            {
                significand *= 10;
            }
            bool const overflow = overflows_double(significand, exponent + digits - 1);
            double const max = ::ducta::numeric_limits<double>::max();
            if (!overflow && !(value <= max))
            {
                value = max;
            }
            else if (overflow && value <= max)
            {
                value = ::ducta::numeric_limits<double>::infinity();
            }
        }
        else
        {
            value = value / powers_of_ten[remainder] * small_powers[steps];
        }
    }
    out = negative ? -value : value;
    return exact;
}

template <class StringType>
bool append_utf8(StringType& out, uint32_t code_point)
{
    char bytes[4];
    size_t count = 0;
    if (code_point < 0x80)
    {
        bytes[count++] = static_cast<char>(code_point);
    }
    else if (code_point < 0x800)
    {
        bytes[count++] = static_cast<char>(0xC0 | (code_point >> 6));
        bytes[count++] = static_cast<char>(0x80 | (code_point & 0x3F));
    }
    else if (code_point < 0x10000)
    {
        bytes[count++] = static_cast<char>(0xE0 | (code_point >> 12));
        bytes[count++] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        bytes[count++] = static_cast<char>(0x80 | (code_point & 0x3F));
    }
    else
    {
        bytes[count++] = static_cast<char>(0xF0 | (code_point >> 18));
        bytes[count++] = static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
        bytes[count++] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        bytes[count++] = static_cast<char>(0x80 | (code_point & 0x3F));
    }
    if (out.size() + count > out.max_size())
    {
        return false;
    }
    out.append(bytes, count);
    return true;
}

DUCTA_DECL bool JsonLoader::load()
{
    jsmn_parser parser;
    jsmn_init(&parser);
    int const count = jsmn_parse(&parser, m_text.data(), m_text.size(), nullptr, 0);
    if (count < 0)
    {
        report_jsmn_error(count, parser.pos);
        return false;
    }
    if (count == 0)
    {
        syntax_error(0, "expected a JSON object");
        return false;
    }

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    if (static_cast<size_t>(count) > m_tokens.size())
    {
        report(
            location(0),
            Path{},
            "too many JSON tokens ({}, max {})",
            count,
            static_cast<unsigned long long>(m_tokens.size()));
        return false;
    }
#else
    m_tokens.resize(static_cast<size_t>(count));
#endif

    jsmn_init(&parser);
    int const parsed = jsmn_parse(
        &parser, m_text.data(), m_text.size(), m_tokens.data(), static_cast<unsigned int>(count));
    if (parsed < 0)
    {
        report_jsmn_error(parsed, parser.pos);
        return false;
    }
    m_count = static_cast<size_t>(parsed);

    Token const& root = m_tokens[0];
    if (root.type != JSMN_OBJECT)
    {
        syntax_error(
            start_of(root),
            "expected an object at the top level, got {}",
            StringView{root.type == JSMN_ARRAY ? "array" : "value"});
        return false;
    }

    // Replace the content of the tree but keep its source name
    SourceName const name{m_tree.source_name().data(), m_tree.source_name().size()};
    m_tree.clear(location(start_of(root)));
    m_tree.set_source_name(StringView{name.data(), name.size()});
    m_file = m_tree.root().location().file_name();

    size_t const next = members(0, m_tree.root().id(), 1);
    if (!m_syntax_ok)
    {
        return false;
    }
    if (next != m_count)
    {
        syntax_error(start_of(m_tokens[next]), "unexpected data after the top-level object");
        return false;
    }
    for (size_t offset = end_of(root); offset < m_text.size(); ++offset)
    {
        if (!is_space(m_text[offset]))
        {
            syntax_error(offset, "unexpected data after the top-level object");
            return false;
        }
    }
    return m_ok;
}

DUCTA_DECL void JsonLoader::report_jsmn_error(int error, size_t offset)
{
    char const c = offset < m_text.size() ? m_text[offset] : '\0';
    if (error == JSMN_ERROR_PART)
    {
        if (c == '"')
        {
            syntax_error(offset, "unterminated string");
        }
        else
        {
            syntax_error(offset, "unexpected end of input");
        }
    }
    else if (error == JSMN_ERROR_INVAL && c == '"')
    {
        syntax_error(offset, "invalid escape sequence in string");
    }
    else if (error == JSMN_ERROR_INVAL && offset < m_text.size())
    {
        report_unexpected(offset);
    }
    else
    {
        syntax_error(offset, "invalid JSON");
    }
}

DUCTA_DECL void JsonLoader::report_unexpected(size_t offset)
{
    unsigned char const c = static_cast<unsigned char>(m_text[offset]);
    if (c >= 0x20 && c < 0x7F)
    {
        syntax_error(offset, "unexpected character '{}'", StringView{m_text.data() + offset, 1});
    }
    else
    {
        syntax_error(offset, "unexpected character (code {})", static_cast<unsigned>(c));
    }
}

/*
 * Text between two tokens: whitespace with exactly one separator (if any expected);
 * closing is the bracket that ends the gap (for better messages)
 */
DUCTA_DECL bool JsonLoader::expect_gap(size_t from, size_t to, char separator, char closing)
{
    size_t offset = from;
    while (offset < to && is_space(m_text[offset]))
    {
        ++offset;
    }
    if (separator != '\0')
    {
        if (offset >= to || m_text[offset] != separator)
        {
            syntax_error(offset, "expected '{}'", StringView{&separator, 1});
            return false;
        }
        ++offset;
        while (offset < to && is_space(m_text[offset]))
        {
            ++offset;
        }
    }
    if (offset < to)
    {
        if (closing != '\0' && m_text[offset] == ',')
        {
            syntax_error(offset, "unexpected ',' before '{}'", StringView{&closing, 1});
        }
        else if (closing != '\0')
        {
            syntax_error(offset, "expected ',' or '{}'", StringView{&closing, 1});
        }
        else
        {
            report_unexpected(offset);
        }
        return false;
    }
    return true;
}

DUCTA_DECL Path JsonLoader::child_path(NodeId parent, StringView key, size_t position) const
{
    Path path;
    if (parent == kInvalidNodeId)
    {
        return path;
    }
    PropNode const node{m_tree, parent};
    path = node.path();
    if (node.is_array())
    {
        String<24> element;
        format_to(element, "[{}]", static_cast<unsigned long long>(position));
        path.append(element.data(), element.size());
    }
    else
    {
        if (!path.empty())
        {
            path.append(".", 1);
        }
        path.append(key.data(), key.size());
    }
    return path;
}

DUCTA_DECL NodeId
JsonLoader::added(Expected<NodeId, Error> result, SourceLocation location, Path const& path)
{
    if (!result)
    {
        report(
            location,
            path,
            "{}",
            StringView{result.error().message.data(), result.error().message.size()});
        return kInvalidNodeId;
    }
    return *result;
}

/* Adds the value of token index to parent (kInvalidNodeId: validate only); returns the next token */
DUCTA_DECL size_t
JsonLoader::value(size_t index, NodeId parent, StringView key, size_t position, size_t depth)
{
    Token const& token = m_tokens[index];
    Path const path = child_path(parent, key, position);
    SourceLocation const value_location = location(start_of(token));

    switch (token.type)
    {
    case JSMN_OBJECT:
    case JSMN_ARRAY: {
        if (depth > m_options.max_depth)
        {
            syntax_error(
                start_of(token),
                "nesting is too deep (max {})",
                static_cast<unsigned long long>(m_options.max_depth));
            return m_count;
        }
        NodeId id = kInvalidNodeId;
        if (parent != kInvalidNodeId)
        {
            id = added(
                token.type == JSMN_OBJECT ? m_tree.add_object(parent, key, value_location)
                                          : m_tree.add_array(parent, key, value_location),
                value_location,
                path);
        }
        return token.type == JSMN_OBJECT ? members(index, id, depth) : elements(index, id, depth);
    }
    case JSMN_STRING: {
        StringValue text;
        if (decode_string(token, text, path) && parent != kInvalidNodeId)
        {
            added(
                m_tree.add_string(parent, key, StringView{text.data(), text.size()}, value_location),
                value_location,
                path);
        }
        return index + 1;
    }
    case JSMN_PRIMITIVE:
        primitive(token, parent, key, path);
        return index + 1;
    default:
        syntax_error(start_of(token), "invalid JSON");
        return m_count;
    }
}

DUCTA_DECL size_t JsonLoader::members(size_t index, NodeId object, size_t depth)
{
    Token const& token = m_tokens[index];
    size_t next = index + 1;
    size_t previous_end = static_cast<size_t>(token.start) + 1;

    for (int member = 0; member < token.size; ++member)
    {
        if (next >= m_count)
        {
            syntax_error(previous_end, "unexpected end of input");
            return m_count;
        }
        Token const& key_token = m_tokens[next];
        if (!expect_gap(previous_end, start_of(key_token), member == 0 ? '\0' : ','))
        {
            return m_count;
        }
        if (key_token.type != JSMN_STRING)
        {
            syntax_error(start_of(key_token), "expected a string key");
            return m_count;
        }
        if (key_token.size == 0 || next + 1 >= m_count)
        {
            syntax_error(end_of(key_token), "expected ':' followed by a value");
            return m_count;
        }

        Token const& value_token = m_tokens[next + 1];
        if (!expect_gap(end_of(key_token), start_of(value_token), ':'))
        {
            return m_count;
        }

        Key key;
        NodeId parent = object;
        Path const object_path = object != kInvalidNodeId ? PropNode{m_tree, object}.path() : Path{};
        if (!decode_string(key_token, key, object_path))
        {
            if (!m_syntax_ok)
            {
                return m_count;
            }
            parent = kInvalidNodeId;
        }

        StringView const key_view{key.data(), key.size()};
        if (parent != kInvalidNodeId)
        {
            PropNode const existing = PropNode{m_tree, object}.child(key_view);
            if (existing.valid())
            {
                SourceLocation const first = existing.location();
                report(
                    location(start_of(key_token)),
                    child_path(object, key_view, 0),
                    "duplicate key '{}' (first defined at {}:{})",
                    key_view,
                    first.line(),
                    first.column());
                parent = kInvalidNodeId;
            }
        }

        next = value(next + 1, parent, key_view, static_cast<size_t>(member), depth + 1);
        if (!m_syntax_ok)
        {
            return m_count;
        }
        previous_end = end_of(value_token);
    }

    if (!expect_gap(
            previous_end,
            static_cast<size_t>(token.end) - 1,
            '\0',
            previous_end == static_cast<size_t>(token.start) + 1 ? '\0' : '}'))
    {
        return m_count;
    }
    return next;
}

DUCTA_DECL size_t JsonLoader::elements(size_t index, NodeId array, size_t depth)
{
    Token const& token = m_tokens[index];
    size_t next = index + 1;
    size_t previous_end = static_cast<size_t>(token.start) + 1;

    for (int element = 0; element < token.size; ++element)
    {
        if (next >= m_count)
        {
            syntax_error(previous_end, "unexpected end of input");
            return m_count;
        }
        Token const& element_token = m_tokens[next];
        if (!expect_gap(previous_end, start_of(element_token), element == 0 ? '\0' : ','))
        {
            return m_count;
        }
        next = value(next, array, StringView{}, static_cast<size_t>(element), depth + 1);
        if (!m_syntax_ok)
        {
            return m_count;
        }
        previous_end = end_of(element_token);
    }

    if (!expect_gap(
            previous_end,
            static_cast<size_t>(token.end) - 1,
            '\0',
            previous_end == static_cast<size_t>(token.start) + 1 ? '\0' : ']'))
    {
        return m_count;
    }
    return next;
}

DUCTA_DECL void JsonLoader::primitive(Token const& token, NodeId parent, StringView key, Path const& path)
{
    StringView const text =
        m_text.substr(static_cast<size_t>(token.start), static_cast<size_t>(token.end - token.start));
    SourceLocation const value_location = location(static_cast<size_t>(token.start));

    if (text == "true" || text == "false")
    {
        if (parent != kInvalidNodeId)
        {
            added(m_tree.add_bool(parent, key, text == "true", value_location), value_location, path);
        }
        return;
    }
    if (text == "null")
    {
        if (parent != kInvalidNodeId)
        {
            added(m_tree.add_null(parent, key, value_location), value_location, path);
        }
        return;
    }

    bool is_integer = false;
    if (!is_json_number(text, is_integer))
    {
        syntax_error(static_cast<size_t>(token.start), "invalid value '{}'", text);
        return;
    }

    if (is_integer)
    {
        int64_t number = 0;
        if (!parse_integer(text, number))
        {
            report(value_location, path, "integer {} is out of range", text);
            return;
        }
        if (parent != kInvalidNodeId)
        {
            added(m_tree.add_integer(parent, key, number, value_location), value_location, path);
        }
        return;
    }

    double number = 0;
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    parse_float(text, number); // without the C library; see parse_float for the precision
#else
    if (!parse_float(text, number))
    {
        /* Not exact by parse_float: strtod rounds correctly ("C" locale assumed) */
        ::std::string const buffer(text.data(), text.size());
        number = ::std::strtod(buffer.c_str(), nullptr);
    }
#endif
    double const max = ::ducta::numeric_limits<double>::max();
    if (!(number <= max && number >= -max))
    {
        report(value_location, path, "number {} is out of range", text);
        return;
    }
    if (parent != kInvalidNodeId)
    {
        added(m_tree.add_float(parent, key, number, value_location), value_location, path);
    }
}

/* Unescapes a string token into out; reports syntax errors and values that do not fit */
template <class StringType>
bool JsonLoader::decode_string(Token const& token, StringType& out, Path const& path)
{
    size_t const begin = static_cast<size_t>(token.start);
    size_t const end = static_cast<size_t>(token.end);
    bool fits = true;

    out.clear();
    for (size_t i = begin; i < end;)
    {
        unsigned char const c = static_cast<unsigned char>(m_text[i]);
        if (c < 0x20)
        {
            syntax_error(i, "control character in string must be escaped");
            return false;
        }
        if (c != '\\')
        {
            if (out.size() < out.max_size())
            {
                out.push_back(static_cast<char>(c));
            }
            else
            {
                fits = false;
            }
            ++i;
            continue;
        }

        char const escape = i + 1 < end ? m_text[i + 1] : '\0';
        char simple = '\0';
        switch (escape)
        {
        case '"':
        case '\\':
        case '/':
            simple = escape;
            break;
        case 'b':
            simple = '\b';
            break;
        case 'f':
            simple = '\f';
            break;
        case 'n':
            simple = '\n';
            break;
        case 'r':
            simple = '\r';
            break;
        case 't':
            simple = '\t';
            break;
        case 'u':
            break;
        default:
            syntax_error(i, "invalid escape sequence in string");
            return false;
        }

        if (escape != 'u')
        {
            if (out.size() < out.max_size())
            {
                out.push_back(simple);
            }
            else
            {
                fits = false;
            }
            i += 2;
            continue;
        }

        auto hex_at = [&](size_t at, uint32_t& value) {
            if (at + 4 > end)
            {
                return false;
            }
            for (size_t k = at; k < at + 4; ++k)
            {
                if (!is_hex(m_text[k]))
                {
                    return false;
                }
            }
            value = hex_value(m_text.substr(at, 4));
            return true;
        };

        uint32_t code_point = 0;
        if (!hex_at(i + 2, code_point))
        {
            syntax_error(i, "invalid unicode escape in string");
            return false;
        }
        size_t length = 6;
        if (code_point >= 0xD800 && code_point <= 0xDBFF)
        {
            uint32_t low = 0;
            if (i + 7 < end && m_text[i + 6] == '\\' && m_text[i + 7] == 'u' &&
                hex_at(i + 8, low) && low >= 0xDC00 && low <= 0xDFFF)
            {
                code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low - 0xDC00);
                length = 12;
            }
            else
            {
                syntax_error(i, "invalid unicode escape in string (unpaired surrogate)");
                return false;
            }
        }
        else if (code_point >= 0xDC00 && code_point <= 0xDFFF)
        {
            syntax_error(i, "invalid unicode escape in string (unpaired surrogate)");
            return false;
        }

        if (fits && !append_utf8(out, code_point))
        {
            fits = false;
        }
        i += length;
    }

    if (!fits)
    {
        report(
            location(start_of(token)),
            path,
            "string is too long (max {} characters)",
            static_cast<unsigned long long>(out.max_size()));
        return false;
    }
    return true;
}

} // namespace Private

DUCTA_DECL bool loadJson(StringView text, PropTree& tree, Diagnostics& diagnostics, JsonOptions options)
{
    // An UTF-8 byte order mark is not part of the content
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
    {
        text = text.substr(3);
    }
    Private::JsonLoader loader{text, tree, diagnostics, options};
    return loader.load();
}

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
DUCTA_DECL bool loadJsonFile(StringView path, PropTree& tree, Diagnostics& diagnostics, JsonOptions options)
{
    std::string const file_path{path.data(), path.size()};
    std::ifstream file{file_path, std::ios::binary};
    if (!file)
    {
        Diagnostic diagnostic;
        format_to(diagnostic.message, "cannot open file '{}'", path);
        diagnostics.add(::ducta::move(diagnostic));
        return false;
    }
    std::string const text{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};

    tree.set_source_name(path);
    return loadJson(StringView{text.data(), text.size()}, tree, diagnostics, options);
}
#endif

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_PRIVATE_JSON_INL
