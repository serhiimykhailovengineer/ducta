#ifndef DUCTA_CONFIGURATION_PROP_TREE_HPP
#define DUCTA_CONFIGURATION_PROP_TREE_HPP

#include "ducta/Core/SourceLocation.hpp"

#include "ducta/Core/Config.hpp"
#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"

// Capacities of the embedded backend (the standard backend grows dynamically).
#ifndef DUCTA_CONFIG_MAX_NODES
#define DUCTA_CONFIG_MAX_NODES 128
#endif

#ifndef DUCTA_CONFIG_MAX_KEY_LENGTH
#define DUCTA_CONFIG_MAX_KEY_LENGTH 32
#endif

#ifndef DUCTA_CONFIG_MAX_STRING_LENGTH
#define DUCTA_CONFIG_MAX_STRING_LENGTH 64
#endif

#ifndef DUCTA_CONFIG_MAX_PATH_LENGTH
#define DUCTA_CONFIG_MAX_PATH_LENGTH 128
#endif

#ifndef DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH
#define DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH 64
#endif

namespace ducta {
namespace Configuration {

using NodeId = uint16_t;
constexpr NodeId kInvalidNodeId = 0xFFFF;

using Key = String<DUCTA_CONFIG_MAX_KEY_LENGTH>;
using StringValue = String<DUCTA_CONFIG_MAX_STRING_LENGTH>;
using Path = String<DUCTA_CONFIG_MAX_PATH_LENGTH>;
using SourceName = String<DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH>;

enum class NodeKind : uint8_t
{
    Null,
    Bool,
    Integer,
    Float,
    String,
    Object,
    Array
};

constexpr StringView to_string(NodeKind kind) noexcept
{
    switch (kind)
    {
    case NodeKind::Null:
        return "null";
    case NodeKind::Bool:
        return "bool";
    case NodeKind::Integer:
        return "integer";
    case NodeKind::Float:
        return "float";
    case NodeKind::String:
        return "string";
    case NodeKind::Object:
        return "object";
    case NodeKind::Array:
        return "array";
    }
    return "unknown";
}

class PropTree;

/*
 * @brief Lightweight read-only handle to a node of a PropTree
 * @note A default constructed (or not found) handle is invalid; every query on an
 *       invalid handle returns an empty / default value.
 */
class PropNode
{
public:
    class Iterator
    {
    public:
        Iterator(PropTree const* tree, NodeId id) noexcept
            : m_tree{tree}
            , m_id{id}
        {
        }

        PropNode operator*() const noexcept;
        Iterator& operator++() noexcept;

        friend bool operator==(Iterator const& a, Iterator const& b) noexcept
        {
            return a.m_id == b.m_id;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) noexcept
        {
            return a.m_id != b.m_id;
        }

    private:
        PropTree const* m_tree;
        NodeId m_id;
    };

public:
    PropNode() = default;
    PropNode(PropTree const& tree, NodeId id) noexcept
        : m_tree{&tree}
        , m_id{id}
    {
    }

    bool valid() const noexcept
    {
        return m_tree != nullptr && m_id != kInvalidNodeId;
    }

    explicit operator bool() const noexcept
    {
        return valid();
    }

    NodeId id() const noexcept
    {
        return m_id;
    }

    NodeKind kind() const noexcept;
    SourceLocation location() const noexcept;

    /* Key in the parent object; empty for the root and for array elements */
    StringView key() const noexcept;

    bool is_null() const noexcept
    {
        return valid() && kind() == NodeKind::Null;
    }

    bool is_object() const noexcept
    {
        return valid() && kind() == NodeKind::Object;
    }

    bool is_array() const noexcept
    {
        return valid() && kind() == NodeKind::Array;
    }

    /* Number of children of an object or array */
    size_t size() const noexcept;

    /* Child of an object by key (invalid handle if missing or not an object) */
    PropNode child(StringView key) const noexcept;

    /* Child of an object or array by location (invalid handle if out of range) */
    PropNode at(size_t index) const noexcept;

    PropNode parent() const noexcept;

    Iterator begin() const noexcept;
    Iterator end() const noexcept;

    /* Scalar values; the node kind must match (checked by assertion) */
    bool as_bool() const noexcept;
    int64_t as_integer() const noexcept;
    double as_float() const noexcept;
    StringView as_string() const noexcept;

    /* Human readable location in the tree, e.g. "filter.taps[2]"; empty for the root */
    Path path() const;

private:
    PropTree const* m_tree{nullptr};
    NodeId m_id{kInvalidNodeId};
};

/*
 * @brief Format independent tree of configuration values
 *
 * Loaders (json / yaml / xml) build the tree with the add_* functions and record where
 * every value came from, so that a configuration which parses fine but is semantically
 * wrong can be reported with its exact file, line and column.
 *
 * The file name is stored once in the tree (set_source_name) and nodes keep only their
 * line / column: PropNode::location() combines both, its function_name() is always empty.
 * Locations point to the tree's source name, so the tree must outlive them.
 *
 * Nodes are stored in a single flat array (no per-node allocation), which keeps the
 * tree usable with the embedded backend.
 */
class PropTree
{
public:
    /* The tree always starts with an empty root object */
    explicit PropTree(SourceLocation root_location = {});

    PropNode root() const noexcept
    {
        return PropNode{*this, 0};
    }

    /* Total number of nodes (including the root) */
    size_t size() const noexcept
    {
        return m_entries.size();
    }

    /* Remove everything (including the source name) but an empty root object */
    void clear(SourceLocation root_location = {});

    /*
     * @brief Name of the source the tree was loaded from (e.g. file path); empty for streams
     * @note With the embedded backend a name longer than DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH
     *       keeps only its trailing characters (the file name is the most useful part).
     */
    void set_source_name(StringView name);

    StringView source_name() const noexcept
    {
        return StringView{m_source_name.data(), m_source_name.size()};
    }

    /*
     * @brief Add a child to an object (key required) or an array (key must be empty)
     * @note Only line and column of location are stored, the file name is the tree's
     *       source name.
     * @return Id of the new node, or an Error if the parent is not a container, the key
     *         is duplicated or a capacity is exceeded
     */
    Expected<NodeId, Error> add_object(NodeId parent, StringView key, SourceLocation location = {});
    Expected<NodeId, Error> add_array(NodeId parent, StringView key, SourceLocation location = {});
    Expected<NodeId, Error> add_null(NodeId parent, StringView key, SourceLocation location = {});
    Expected<NodeId, Error> add_bool(NodeId parent, StringView key, bool value, SourceLocation location = {});
    Expected<NodeId, Error> add_integer(
        NodeId parent,
        StringView key,
        int64_t value,
        SourceLocation location = {});
    Expected<NodeId, Error> add_float(
        NodeId parent,
        StringView key,
        double value,
        SourceLocation location = {});
    Expected<NodeId, Error> add_string(
        NodeId parent,
        StringView key,
        StringView value,
        SourceLocation location = {});

private:
    friend class PropNode;

    struct Entry
    {
        Key key;
        StringValue string_value;
        uint_least32_t line{0};
        uint_least32_t column{0};
        NodeKind kind{NodeKind::Null};
        NodeId parent{kInvalidNodeId};
        NodeId first_child{kInvalidNodeId};
        NodeId last_child{kInvalidNodeId};
        NodeId next_sibling{kInvalidNodeId};
        uint16_t child_count{0};
        bool bool_value{false};
        int64_t integer_value{0};
        double float_value{0.0};
    };

    Expected<NodeId, Error> add_entry(NodeId parent, StringView key, NodeKind kind, SourceLocation location);

    Entry const& entry(NodeId id) const noexcept
    {
        return m_entries[id];
    }

private:
    Vector<Entry, DUCTA_CONFIG_MAX_NODES> m_entries;
    SourceName m_source_name;
};

} // namespace Configuration
} // namespace ducta

#if !defined(DUCTA_SEPARATE_COMPILATION)
#include "ducta/Configuration/Private/PropTree.inl"
#endif

#endif // DUCTA_CONFIGURATION_PROP_TREE_HPP
