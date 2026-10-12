#ifndef DUCTA_CONFIGURATION_DIAGNOSTICS_HPP
#define DUCTA_CONFIGURATION_DIAGNOSTICS_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/SourceLocation.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {
namespace Configuration {

enum class Severity : uint8_t
{
    /* The configuration cannot be used (decoding fails) */
    Error,
    /* Suspicious but harmless (e.g. an unknown field, which is ignored) */
    Warning
};

constexpr StringView to_string(Severity severity) noexcept
{
    return severity == Severity::Warning ? StringView{"warning"} : StringView{"error"};
}

/*
 * @brief A problem found in a configuration that was loaded successfully,
 *        e.g. a wrong value type, a value out of range (errors) or an unknown field (warning)
 */
struct Diagnostic
{
    Severity severity{Severity::Error};
    /* Where in the configuration source; file_name() points into the PropTree */
    SourceLocation location;
    Path path;
    String<100> message;
    /* Where in the C++ code the problem was reported (e.g. which config rule) */
    SourceLocation origin;
};

/* Prints "file:line:column: path: message" (unknown parts are omitted),
 * warnings are prefixed with "warning: " */
template <class OutputStream>
OutputStream& operator<<(OutputStream& os, Diagnostic const& diagnostic)
{
    if (diagnostic.location.file_name()[0] != '\0')
    {
        os << diagnostic.location.file_name() << ':';
    }
    if (diagnostic.location.valid())
    {
        os << diagnostic.location.line() << ':' << diagnostic.location.column() << ": ";
    }
    if (!diagnostic.path.empty())
    {
        os << diagnostic.path << ": ";
    }
    if (diagnostic.severity == Severity::Warning)
    {
        os << "warning: ";
    }
    os << diagnostic.message;
    return os;
}

/*
 * @brief Collects every configuration problem instead of stopping at the first one
 * @note With the embedded backend at most DUCTA_CONFIG_MAX_DIAGNOSTICS are kept;
 *       the rest are counted in dropped(). error_count() / warning_count() count all
 *       reported diagnostics, dropped ones included.
 */
class Diagnostics
{
public:
    using Container = Vector<Diagnostic, DUCTA_CONFIG_MAX_DIAGNOSTICS>;

    void add(Diagnostic diagnostic)
    {
        if (diagnostic.severity == Severity::Warning)
        {
            ++m_warnings;
        }
        else
        {
            ++m_errors;
        }
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
        if (m_items.full())
        {
            ++m_dropped;
            return;
        }
#endif
        m_items.push_back(::ducta::move(diagnostic));
    }

    /* Report a problem at the location of node */
    void error(PropNode node, StringView message, SourceLocation origin = SourceLocation::current())
    {
        Diagnostic diagnostic = make(node);
        diagnostic.origin = origin;
        diagnostic.message.assign(message.data(), message.size());
        add(::ducta::move(diagnostic));
    }

    /* Report a problem at the location of node with a formatted message
     * (origin is not captured: a default argument cannot follow the parameter pack) */
    template <class... Args>
    void errorf(PropNode node, format_string<Args...> fmt, Args&&... args)
    {
        Diagnostic diagnostic = make(node);
        format_to(diagnostic.message, fmt, ::ducta::forward<Args>(args)...);
        add(::ducta::move(diagnostic));
    }

    /* Report a harmless problem at the location of node */
    void warning(PropNode node, StringView message, SourceLocation origin = SourceLocation::current())
    {
        Diagnostic diagnostic = make(node);
        diagnostic.severity = Severity::Warning;
        diagnostic.origin = origin;
        diagnostic.message.assign(message.data(), message.size());
        add(::ducta::move(diagnostic));
    }

    template <class... Args>
    void warningf(PropNode node, format_string<Args...> fmt, Args&&... args)
    {
        Diagnostic diagnostic = make(node);
        diagnostic.severity = Severity::Warning;
        format_to(diagnostic.message, fmt, ::ducta::forward<Args>(args)...);
        add(::ducta::move(diagnostic));
    }

    /* No diagnostics at all (neither errors nor warnings) */
    bool empty() const noexcept
    {
        return m_items.empty() && m_dropped == 0;
    }

    bool has_errors() const noexcept
    {
        return m_errors != 0;
    }

    size_t error_count() const noexcept
    {
        return m_errors;
    }

    size_t warning_count() const noexcept
    {
        return m_warnings;
    }

    size_t size() const noexcept
    {
        return m_items.size();
    }

    /* Number of diagnostics that did not fit into the container */
    size_t dropped() const noexcept
    {
        return m_dropped;
    }

    Diagnostic const& operator[](size_t index) const
    {
        return m_items[index];
    }

    auto begin() const noexcept
    {
        return m_items.begin();
    }

    auto end() const noexcept
    {
        return m_items.end();
    }

    void clear() noexcept
    {
        m_items.clear();
        m_dropped = 0;
        m_errors = 0;
        m_warnings = 0;
    }

private:
    static Diagnostic make(PropNode node)
    {
        Diagnostic diagnostic;
        diagnostic.location = node.location();
        diagnostic.path = node.path();
        return diagnostic;
    }

private:
    Container m_items;
    size_t m_dropped{0};
    size_t m_errors{0};
    size_t m_warnings{0};
};

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_DIAGNOSTICS_HPP
