#ifndef DUCTA_CONFIGURATION_DECODE_HPP
#define DUCTA_CONFIGURATION_DECODE_HPP

#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Limits.hpp"
#include "ducta/Core/SourceLocation.hpp"
#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {
namespace Configuration {

/*
 * @brief Maps the fields of a configuration struct to object members.
 * @note Specialised by DEFINE_CONFIG (see DefineConfig.hpp).
 */
template <class T>
struct ConfigTraits
{
};

/*
 * @brief Decodes a single value. Specialise it to support custom types:
 *        static bool decode(PropNode node, T& out, Diagnostics& diagnostics);
 */
template <class T, class Enable = void>
struct ValueDecoder
{
};

/*
 * @brief Semantic rules of an already decoded configuration struct, kept outside the struct.
 * @note Specialised by DEFINE_CONFIG_RULES (see DefineConfig.hpp), or by hand for rules
 *       that do not fit a single condition:
 *        static void validate(T const& config, Validator& validator);
 */
template <class T>
struct ConfigRules
{
};

/*
 * @brief Reports rule violations of a decoded configuration (used by ConfigRules).
 *
 * Errors are reported at the location of the named field (or of the struct itself when the
 * field is absent), so that a configuration that parsed fine but holds wrong values points
 * to the right line.
 */
class Validator
{
public:
    Validator(PropNode node, Diagnostics& diagnostics) noexcept
        : m_node{node}
        , m_diagnostics{diagnostics}
    {
    }

    /* Node of the struct being validated */
    PropNode node() const noexcept
    {
        return m_node;
    }

    bool has(StringView field) const noexcept
    {
        return m_node.child(field).valid();
    }

    /* Report an error if field is not present in the configuration */
    bool require(StringView field, SourceLocation origin = SourceLocation::current())
    {
        if (has(field))
        {
            return true;
        }
        Diagnostic diagnostic = make(m_node, origin);
        format_to(diagnostic.message, "missing required field '{}'", field);
        m_diagnostics.add(::ducta::move(diagnostic));
        m_ok = false;
        return false;
    }

    /* Report message at field when condition is false; returns condition */
    bool check(
        bool condition,
        StringView field,
        StringView message,
        SourceLocation origin = SourceLocation::current())
    {
        if (!condition)
        {
            error(field, message, origin);
        }
        return condition;
    }

    /* Report message at field (or at the struct when the field is absent) */
    void error(StringView field, StringView message, SourceLocation origin = SourceLocation::current())
    {
        PropNode child = m_node.child(field);
        if (child.valid())
        {
            m_diagnostics.error(child, message, origin);
        }
        else
        {
            Diagnostic diagnostic = make(m_node, origin);
            if (!diagnostic.path.empty())
            {
                diagnostic.path.append(".", 1);
            }
            diagnostic.path.append(field.data(), field.size());
            diagnostic.message.assign(message.data(), message.size());
            m_diagnostics.add(::ducta::move(diagnostic));
        }
        m_ok = false;
    }

    /* Report message at the struct itself */
    void error(StringView message, SourceLocation origin = SourceLocation::current())
    {
        m_diagnostics.error(m_node, message, origin);
        m_ok = false;
    }

    bool ok() const noexcept
    {
        return m_ok;
    }

private:
    static Diagnostic make(PropNode node, SourceLocation origin)
    {
        Diagnostic diagnostic;
        diagnostic.location = node.location();
        diagnostic.path = node.path();
        diagnostic.origin = origin;
        return diagnostic;
    }

private:
    PropNode m_node;
    Diagnostics& m_diagnostics;
    bool m_ok{true};
};

namespace Private {

template <class T, class = void>
struct has_config_traits : ::ducta::false_type
{
};

template <class T>
struct has_config_traits<
    T,
    ::ducta::void_t<decltype(ConfigTraits<T>::decode(
        ::ducta::declval<PropNode>(),
        ::ducta::declval<T&>(),
        ::ducta::declval<Diagnostics&>()))>> : ::ducta::true_type
{
};

template <class T, class = void>
struct has_value_decoder : ::ducta::false_type
{
};

template <class T>
struct has_value_decoder<
    T,
    ::ducta::void_t<decltype(ValueDecoder<T>::decode(
        ::ducta::declval<PropNode>(),
        ::ducta::declval<T&>(),
        ::ducta::declval<Diagnostics&>()))>> : ::ducta::true_type
{
};

template <class T, class = void>
struct has_config_rules : ::ducta::false_type
{
};

template <class T>
struct has_config_rules<
    T,
    ::ducta::void_t<
        decltype(ConfigRules<T>::validate(::ducta::declval<T const&>(), ::ducta::declval<Validator&>()))>>
    : ::ducta::true_type
{
};

template <class T>
bool validate(PropNode node, T const& value, Diagnostics& diagnostics)
{
    if constexpr (has_config_rules<T>::value)
    {
        Validator validator{node, diagnostics};
        ConfigRules<T>::validate(value, validator);
        return validator.ok();
    }
    return true;
}

} // namespace Private

/*
 * @brief Decode node into out (ConfigTraits / ValueDecoder), then apply ConfigRules if any,
 *        reporting every problem to diagnostics
 * @return true if the value was decoded and validated without errors
 */
template <class T>
bool decode(PropNode node, T& out, Diagnostics& diagnostics)
{
    if constexpr (Private::has_config_traits<T>::value)
    {
        return ConfigTraits<T>::decode(node, out, diagnostics) &&
               Private::validate(node, static_cast<T const&>(out), diagnostics);
    }
    else if constexpr (Private::has_value_decoder<T>::value)
    {
        return ValueDecoder<T>::decode(node, out, diagnostics) &&
               Private::validate(node, static_cast<T const&>(out), diagnostics);
    }
    else
    {
        static_assert(
            sizeof(T) == 0,
            "No decoder for this type: use DEFINE_CONFIG or specialise "
            "ducta::Configuration::ValueDecoder");
        return false;
    }
}

/*
 * @brief Decode node into a default constructed T
 * @return The decoded value, or an empty Optional if any problem was reported
 */
template <class T>
Optional<T> decode(PropNode node, Diagnostics& diagnostics)
{
    T value{};
    if (!decode(node, value, diagnostics))
    {
        return {};
    }
    return Optional<T>{::ducta::move(value)};
}

namespace Private {

inline bool expect_kind(PropNode node, NodeKind kind, Diagnostics& diagnostics)
{
    if (node.kind() == kind)
    {
        return true;
    }
    diagnostics.errorf(node, "expected {}, got {}", to_string(kind), to_string(node.kind()));
    return false;
}

/* Used by DEFINE_CONFIG to decode the fields of a struct */
class ObjectDecoder
{
public:
    ObjectDecoder(PropNode node, Diagnostics& diagnostics)
        : m_node{node}
        , m_diagnostics{diagnostics}
        , m_ok{expect_kind(node, NodeKind::Object, diagnostics)}
    {
    }

    bool valid() const noexcept
    {
        return m_node.is_object();
    }

    /* Missing fields keep their default value */
    template <class T>
    void field(StringView name, T& member)
    {
        PropNode child = m_node.child(name);
        if (child.valid() && !::ducta::Configuration::decode(child, member, m_diagnostics))
        {
            m_ok = false;
        }
    }

    /* Report object members that do not correspond to any field (typos, stale keys) */
    template <size_t N>
    void reject_unknown(StringView const (&names)[N])
    {
        for (PropNode child : m_node)
        {
            bool known = false;
            for (StringView name : names)
            {
                if (child.key() == name)
                {
                    known = true;
                    break;
                }
            }
            if (!known)
            {
                m_diagnostics.errorf(child, "unknown field '{}'", child.key());
                m_ok = false;
            }
        }
    }

    bool ok() const noexcept
    {
        return m_ok;
    }

private:
    PropNode m_node;
    Diagnostics& m_diagnostics;
    bool m_ok;
};

} // namespace Private

// ========== Built-in value decoders ==========

template <>
struct ValueDecoder<bool>
{
    static bool decode(PropNode node, bool& out, Diagnostics& diagnostics)
    {
        if (!Private::expect_kind(node, NodeKind::Bool, diagnostics))
        {
            return false;
        }
        out = node.as_bool();
        return true;
    }
};

template <class T>
struct ValueDecoder<T, ::ducta::enable_if_t<::ducta::is_integral_v<T> && !::ducta::is_same_v<T, bool>>>
{
    static bool decode(PropNode node, T& out, Diagnostics& diagnostics)
    {
        if (!Private::expect_kind(node, NodeKind::Integer, diagnostics))
        {
            return false;
        }

        int64_t const value = node.as_integer();
        bool in_range = true;
        if constexpr (::ducta::is_signed_v<T>)
        {
            in_range = value >= static_cast<int64_t>(::ducta::numeric_limits<T>::min()) &&
                       value <= static_cast<int64_t>(::ducta::numeric_limits<T>::max());
        }
        else
        {
            in_range = value >= 0 && static_cast<uint64_t>(value) <=
                                         static_cast<uint64_t>(::ducta::numeric_limits<T>::max());
        }

        if (!in_range)
        {
            if constexpr (::ducta::is_signed_v<T>)
            {
                diagnostics.errorf(
                    node,
                    "value {} is out of range [{}, {}]",
                    static_cast<long long>(value),
                    static_cast<long long>(::ducta::numeric_limits<T>::min()),
                    static_cast<long long>(::ducta::numeric_limits<T>::max()));
            }
            else
            {
                diagnostics.errorf(
                    node,
                    "value {} is out of range [0, {}]",
                    static_cast<long long>(value),
                    static_cast<unsigned long long>(::ducta::numeric_limits<T>::max()));
            }
            return false;
        }

        out = static_cast<T>(value);
        return true;
    }
};

/* Accepts both float and integer values */
template <class T>
struct ValueDecoder<T, ::ducta::enable_if_t<::ducta::is_floating_point_v<T>>>
{
    static bool decode(PropNode node, T& out, Diagnostics& diagnostics)
    {
        if (node.kind() == NodeKind::Integer)
        {
            out = static_cast<T>(node.as_integer());
            return true;
        }
        if (!Private::expect_kind(node, NodeKind::Float, diagnostics))
        {
            return false;
        }
        out = static_cast<T>(node.as_float());
        return true;
    }
};

namespace Private {

template <class StringType>
bool decode_string(PropNode node, StringType& out, Diagnostics& diagnostics)
{
    if (!expect_kind(node, NodeKind::String, diagnostics))
    {
        return false;
    }
    StringView value = node.as_string();
    if (value.size() > out.max_size())
    {
        diagnostics.errorf(
            node,
            "string is too long ({} characters, max {})",
            static_cast<unsigned long long>(value.size()),
            static_cast<unsigned long long>(out.max_size()));
        return false;
    }
    out.assign(value.data(), value.size());
    return true;
}

template <class VectorType>
bool decode_vector(PropNode node, VectorType& out, Diagnostics& diagnostics)
{
    if (!expect_kind(node, NodeKind::Array, diagnostics))
    {
        return false;
    }
    if (node.size() > out.max_size())
    {
        diagnostics.errorf(
            node,
            "too many elements ({}, max {})",
            static_cast<unsigned long long>(node.size()),
            static_cast<unsigned long long>(out.max_size()));
        return false;
    }

    out.clear();
    bool ok = true;
    for (PropNode element : node)
    {
        typename VectorType::value_type value{};
        if (::ducta::Configuration::decode(element, value, diagnostics))
        {
            out.push_back(::ducta::move(value));
        }
        else
        {
            ok = false;
        }
    }
    return ok;
}

} // namespace Private

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

template <size_t N>
struct ValueDecoder<etl::string<N>>
{
    static bool decode(PropNode node, etl::string<N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_string(node, out, diagnostics);
    }
};

template <class T, size_t N>
struct ValueDecoder<etl::vector<T, N>>
{
    static bool decode(PropNode node, etl::vector<T, N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_vector(node, out, diagnostics);
    }
};

#else

template <>
struct ValueDecoder<std::string>
{
    static bool decode(PropNode node, std::string& out, Diagnostics& diagnostics)
    {
        return Private::decode_string(node, out, diagnostics);
    }
};

template <class T>
struct ValueDecoder<std::vector<T>>
{
    static bool decode(PropNode node, std::vector<T>& out, Diagnostics& diagnostics)
    {
        return Private::decode_vector(node, out, diagnostics);
    }
};

#endif

/* null resets the optional, any other value is decoded into it */
template <class T>
struct ValueDecoder<Optional<T>>
{
    static bool decode(PropNode node, Optional<T>& out, Diagnostics& diagnostics)
    {
        if (node.is_null())
        {
            out.reset();
            return true;
        }
        T value{};
        if (!::ducta::Configuration::decode(node, value, diagnostics))
        {
            return false;
        }
        out = ::ducta::move(value);
        return true;
    }
};

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_DECODE_HPP
