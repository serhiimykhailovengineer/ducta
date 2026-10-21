#ifndef DUCTA_CONFIGURATION_DECODE_HPP
#define DUCTA_CONFIGURATION_DECODE_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Limits.hpp"
#include "ducta/Core/SourceLocation.hpp"
#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Array.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Variant.hpp"
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
 *        static constexpr bool accepts(NodeKind kind); // optional: kinds the type is
 *                                                      // decoded from (used by variants)
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

template <class T, class = void>
struct has_accepts : ::ducta::false_type
{
};

template <class T>
struct has_accepts<T, ::ducta::void_t<decltype(ValueDecoder<T>::accepts(NodeKind::Null))>>
    : ::ducta::true_type
{
};

/*
 * Whether T can be decoded from a value of the given kind: DEFINE_CONFIG structs (and
 * tagged variants) come from objects, value decoders tell via accepts(); a custom decoder
 * without accepts() is assumed to take any kind.
 */
template <class T>
constexpr bool accepts_kind(NodeKind kind) noexcept
{
    if constexpr (has_config_traits<T>::value)
    {
        return kind == NodeKind::Object;
    }
    else if constexpr (has_accepts<T>::value)
    {
        return ValueDecoder<T>::accepts(kind);
    }
    else
    {
        return true;
    }
}

} // namespace Private

/*
 * @brief Apply ConfigRules of T (if any) to an already decoded value
 * @return true if no rule was violated
 */
template <class T>
bool validate(PropNode node, T const& value, Diagnostics& diagnostics)
{
    if constexpr (Private::has_config_rules<T>::value)
    {
        Validator validator{node, diagnostics};
        ConfigRules<T>::validate(value, validator);
        return validator.ok();
    }
    return true;
}

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
               validate(node, static_cast<T const&>(out), diagnostics);
    }
    else if constexpr (Private::has_value_decoder<T>::value)
    {
        return ValueDecoder<T>::decode(node, out, diagnostics) &&
               validate(node, static_cast<T const&>(out), diagnostics);
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

    /* Report object members that do not correspond to any field (typos, stale keys);
     * ignored is a key handled by the caller (the tag of a tagged variant) */
    template <size_t N>
    void warn_unknown(StringView const (&names)[N], StringView ignored = {})
    {
        for (PropNode child : m_node)
        {
            bool known = !ignored.empty() && child.key() == ignored;
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
                /* Unknown fields are ignored: a warning, decoding still succeeds */
                m_diagnostics.warningf(child, "unknown field '{}'", child.key());
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

/*
 * A PropNode member keeps the raw subtree (any kind) to be decoded later, e.g. the
 * configuration of a node whose type is known only at run time. The handle points into the
 * PropTree, which must outlive it.
 */
template <>
struct ValueDecoder<PropNode>
{
    static bool decode(PropNode node, PropNode& out, Diagnostics& /*diagnostics*/)
    {
        out = node;
        return true;
    }
};

template <>
struct ValueDecoder<bool>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Bool;
    }

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
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Integer;
    }

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
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Integer || kind == NodeKind::Float;
    }

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
        double const value = node.as_float();
        double const max = static_cast<double>(::ducta::numeric_limits<T>::max());
        if (!(value <= max && value >= -max))
        {
            diagnostics.errorf(node, "value is out of range for destination floating-point type");
            return false;
        }
        out = static_cast<T>(value);
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

/* Fixed size: the array must have exactly as many elements as ArrayType */
template <class ArrayType>
bool decode_array(PropNode node, ArrayType& out, Diagnostics& diagnostics)
{
    if (!expect_kind(node, NodeKind::Array, diagnostics))
    {
        return false;
    }
    if (node.size() != out.size())
    {
        diagnostics.errorf(
            node,
            "expected {} elements, got {}",
            static_cast<unsigned long long>(out.size()),
            static_cast<unsigned long long>(node.size()));
        return false;
    }

    bool ok = true;
    size_t index = 0;
    for (PropNode element : node)
    {
        typename ArrayType::value_type value{};
        if (::ducta::Configuration::decode(element, value, diagnostics))
        {
            out[index] = ::ducta::move(value);
        }
        else
        {
            ok = false;
        }
        ++index;
    }
    return ok;
}

template <class T, class = void>
struct is_string_key : ::ducta::false_type
{
};

template <class T>
struct is_string_key<
    T,
    ::ducta::void_t<
        decltype(::ducta::declval<T&>().assign(::ducta::declval<char const*>(), size_t{})),
        decltype(::ducta::declval<T const&>().max_size())>> : ::ducta::true_type
{
};

/* Object members become map entries: the member key is the map key */
template <class MapType>
bool decode_map(PropNode node, MapType& out, Diagnostics& diagnostics)
{
    using Key = typename MapType::key_type;
    using Value = typename MapType::mapped_type;
    static_assert(is_string_key<Key>::value, "Map keys must be strings (String<N>)");

    if (!expect_kind(node, NodeKind::Object, diagnostics))
    {
        return false;
    }
    if (node.size() > out.max_size())
    {
        diagnostics.errorf(
            node,
            "too many entries ({}, max {})",
            static_cast<unsigned long long>(node.size()),
            static_cast<unsigned long long>(out.max_size()));
        return false;
    }

    out.clear();
    bool ok = true;
    for (PropNode child : node)
    {
        StringView const name = child.key();
        Key key{};
        if (name.size() > key.max_size())
        {
            diagnostics.errorf(
                child,
                "key '{}' is too long (max {} characters)",
                name,
                static_cast<unsigned long long>(key.max_size()));
            ok = false;
            continue;
        }
        key.assign(name.data(), name.size());

        Value value{};
        if (::ducta::Configuration::decode(child, value, diagnostics))
        {
            out.insert(::ducta::pair<Key const, Value>(::ducta::move(key), ::ducta::move(value)));
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
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::String;
    }

    static bool decode(PropNode node, etl::string<N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_string(node, out, diagnostics);
    }
};

template <class T, size_t N>
struct ValueDecoder<etl::vector<T, N>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Array;
    }

    static bool decode(PropNode node, etl::vector<T, N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_vector(node, out, diagnostics);
    }
};

template <class T, size_t N>
struct ValueDecoder<etl::array<T, N>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Array;
    }

    static bool decode(PropNode node, etl::array<T, N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_array(node, out, diagnostics);
    }
};

template <class K, class V, size_t N, class Compare>
struct ValueDecoder<etl::map<K, V, N, Compare>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Object;
    }

    static bool decode(PropNode node, etl::map<K, V, N, Compare>& out, Diagnostics& diagnostics)
    {
        return Private::decode_map(node, out, diagnostics);
    }
};

#else

template <>
struct ValueDecoder<std::string>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::String;
    }

    static bool decode(PropNode node, std::string& out, Diagnostics& diagnostics)
    {
        return Private::decode_string(node, out, diagnostics);
    }
};

template <class T>
struct ValueDecoder<std::vector<T>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Array;
    }

    static bool decode(PropNode node, std::vector<T>& out, Diagnostics& diagnostics)
    {
        return Private::decode_vector(node, out, diagnostics);
    }
};

template <class T, size_t N>
struct ValueDecoder<std::array<T, N>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Array;
    }

    static bool decode(PropNode node, std::array<T, N>& out, Diagnostics& diagnostics)
    {
        return Private::decode_array(node, out, diagnostics);
    }
};

template <class K, class V, class Compare, class Allocator>
struct ValueDecoder<std::map<K, V, Compare, Allocator>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Object;
    }

    static bool decode(PropNode node, std::map<K, V, Compare, Allocator>& out, Diagnostics& diagnostics)
    {
        return Private::decode_map(node, out, diagnostics);
    }
};

#endif

/* null resets the optional, any other value is decoded into it */
template <class T>
struct ValueDecoder<Optional<T>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Null || Private::accepts_kind<T>(kind);
    }

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

namespace Private {

/* "integer or string" for every kind accepted by T */
template <class T>
String<100> describe_accepted_kinds()
{
    constexpr NodeKind kinds[] = {
        NodeKind::Null,
        NodeKind::Bool,
        NodeKind::Integer,
        NodeKind::Float,
        NodeKind::String,
        NodeKind::Object,
        NodeKind::Array};

    String<100> result;
    for (NodeKind kind : kinds)
    {
        if (accepts_kind<T>(kind))
        {
            if (!result.empty())
            {
                result.append(" or ", 4);
            }
            StringView name = to_string(kind);
            result.append(name.data(), name.size());
        }
    }
    return result;
}

/* Used by DEFINE_CONFIG_VARIANT: the alternative is selected by a string member (the tag) */
class TaggedVariantDecoder
{
public:
    TaggedVariantDecoder(PropNode node, StringView tag_key, Diagnostics& diagnostics)
        : m_node{node}
        , m_tag_key{tag_key}
        , m_diagnostics{diagnostics}
    {
        if (!expect_kind(node, NodeKind::Object, diagnostics))
        {
            return;
        }
        PropNode tag = node.child(tag_key);
        if (!tag.valid())
        {
            diagnostics.errorf(node, "missing required field '{}'", tag_key);
            return;
        }
        if (!expect_kind(tag, NodeKind::String, diagnostics))
        {
            return;
        }
        m_tag = tag;
    }

    bool valid() const noexcept
    {
        return m_tag.valid();
    }

    StringView tag() const noexcept
    {
        return m_tag.as_string();
    }

    /* Decode the object (without the tag member) as alternative T of out */
    template <class T, class VariantType>
    bool alternative(VariantType& out)
    {
        static_assert(
            has_config_traits<T>::value,
            "Alternatives of a tagged variant must be structs defined with DEFINE_CONFIG");

        T value{};
        if (!ConfigTraits<T>::decode(m_node, value, m_diagnostics, m_tag_key) ||
            !::ducta::Configuration::validate(m_node, static_cast<T const&>(value), m_diagnostics))
        {
            return false;
        }
        out.template emplace<T>(::ducta::move(value));
        return true;
    }

    template <size_t N>
    bool reject_unknown_tag(StringView const (&names)[N])
    {
        String<100> expected;
        for (StringView name : names)
        {
            if (!expected.empty())
            {
                expected.append(", ", 2);
            }
            expected.append(name.data(), name.size());
        }
        m_diagnostics.errorf(
            m_tag,
            "unknown {} '{}' (expected {})",
            m_tag_key,
            tag(),
            StringView{expected.data(), expected.size()});
        return false;
    }

private:
    PropNode m_node;
    StringView m_tag_key;
    Diagnostics& m_diagnostics;
    PropNode m_tag;
};

} // namespace Private

/*
 * Untagged variant: the first alternative that accepts the kind of the value is decoded,
 * e.g. Variant<int32_t, String<16>> takes 5 or "five". Order matters when several
 * alternatives accept the same kind (Variant<double, int32_t> always picks double).
 * Use DEFINE_CONFIG_VARIANT to select between struct alternatives by a tag.
 */
template <class... Ts>
struct ValueDecoder<Variant<Ts...>>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return (Private::accepts_kind<Ts>(kind) || ...);
    }

    static bool decode(PropNode node, Variant<Ts...>& out, Diagnostics& diagnostics)
    {
        return decode_alternative<0, Ts...>(node, out, diagnostics);
    }

private:
    template <size_t Index, class T, class... Rest>
    static bool decode_alternative(PropNode node, Variant<Ts...>& out, Diagnostics& diagnostics)
    {
        if (Private::accepts_kind<T>(node.kind()))
        {
            T value{};
            if (!::ducta::Configuration::decode(node, value, diagnostics))
            {
                return false;
            }
            out.template emplace<Index>(::ducta::move(value));
            return true;
        }

        if constexpr (sizeof...(Rest) > 0)
        {
            return decode_alternative<Index + 1, Rest...>(node, out, diagnostics);
        }
        else
        {
            String<100> expected = Private::describe_accepted_kinds<Variant<Ts...>>();
            diagnostics.errorf(
                node,
                "expected {}, got {}",
                StringView{expected.data(), expected.size()},
                to_string(node.kind()));
            return false;
        }
    }
};

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_DECODE_HPP
