#ifndef DUCTA_CONFIGURATION_PRIVATE_PROP_TREE_INL
#define DUCTA_CONFIGURATION_PRIVATE_PROP_TREE_INL

#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Assert.hpp"
#include "ducta/Core/Config.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {
namespace Configuration {

namespace Private {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
constexpr size_t kMaxNodes =
    DUCTA_CONFIG_MAX_NODES < kInvalidNodeId ? DUCTA_CONFIG_MAX_NODES : kInvalidNodeId;
#else
constexpr size_t kMaxNodes = kInvalidNodeId;
#endif

template <class... Args>
Error make_error(format_string<Args...> fmt, Args&&... args)
{
    Error error;
    format_to(error.message, fmt, ::ducta::forward<Args>(args)...);
    return error;
}

template <class StringType>
void append(StringType& target, StringView text)
{
    target.append(text.data(), text.size());
}

} // namespace Private

// ========== PropNode ==========

DUCTA_DECL PropNode PropNode::Iterator::operator*() const noexcept
{
    return PropNode{*m_tree, m_id};
}

DUCTA_DECL PropNode::Iterator& PropNode::Iterator::operator++() noexcept
{
    m_id = m_tree->entry(m_id).next_sibling;
    return *this;
}

DUCTA_DECL NodeKind PropNode::kind() const noexcept
{
    return valid() ? m_tree->entry(m_id).kind : NodeKind::Null;
}

DUCTA_DECL SourceLocation PropNode::location() const noexcept
{
    if (!valid())
    {
        return SourceLocation{};
    }
    auto const& entry = m_tree->entry(m_id);
    return SourceLocation{entry.line, entry.column, m_tree->m_source_name.c_str()};
}

DUCTA_DECL StringView PropNode::key() const noexcept
{
    if (!valid())
    {
        return StringView{};
    }
    auto const& key = m_tree->entry(m_id).key;
    return StringView{key.data(), key.size()};
}

DUCTA_DECL size_t PropNode::size() const noexcept
{
    return valid() ? m_tree->entry(m_id).child_count : 0;
}

DUCTA_DECL PropNode PropNode::child(StringView key) const noexcept
{
    if (!is_object())
    {
        return PropNode{};
    }
    for (PropNode node : *this)
    {
        if (node.key() == key)
        {
            return node;
        }
    }
    return PropNode{};
}

DUCTA_DECL PropNode PropNode::at(size_t index) const noexcept
{
    if (index >= size())
    {
        return PropNode{};
    }
    auto it = begin();
    for (size_t i = 0; i < index; ++i)
    {
        ++it;
    }
    return *it;
}

DUCTA_DECL PropNode PropNode::parent() const noexcept
{
    if (!valid() || m_tree->entry(m_id).parent == kInvalidNodeId)
    {
        return PropNode{};
    }
    return PropNode{*m_tree, m_tree->entry(m_id).parent};
}

DUCTA_DECL PropNode::Iterator PropNode::begin() const noexcept
{
    if (!valid())
    {
        return Iterator{m_tree, kInvalidNodeId};
    }
    return Iterator{m_tree, m_tree->entry(m_id).first_child};
}

DUCTA_DECL PropNode::Iterator PropNode::end() const noexcept
{
    return Iterator{m_tree, kInvalidNodeId};
}

DUCTA_DECL bool PropNode::as_bool() const noexcept
{
    DUCTA_ASSERT(kind() == NodeKind::Bool);
    return valid() ? m_tree->entry(m_id).bool_value : false;
}

DUCTA_DECL int64_t PropNode::as_integer() const noexcept
{
    DUCTA_ASSERT(kind() == NodeKind::Integer);
    return valid() ? m_tree->entry(m_id).integer_value : 0;
}

DUCTA_DECL double PropNode::as_float() const noexcept
{
    DUCTA_ASSERT(kind() == NodeKind::Float);
    return valid() ? m_tree->entry(m_id).float_value : 0.0;
}

DUCTA_DECL StringView PropNode::as_string() const noexcept
{
    DUCTA_ASSERT(kind() == NodeKind::String);
    if (!valid())
    {
        return StringView{};
    }
    auto const& value = m_tree->entry(m_id).string_value;
    return StringView{value.data(), value.size()};
}

DUCTA_DECL Path PropNode::path() const
{
    Path result;
    if (!valid())
    {
        return result;
    }

    // The path is built from this node up to the root with every segment written backwards,
    // then reversed as a whole: no list of ancestors is needed, so any depth is supported.
    // A path that does not fit (embedded backend) keeps its end and starts with "...".
    constexpr StringView kEllipsis = "...";
    size_t const capacity = result.max_size() - kEllipsis.size();
    bool truncated = false;

    for (NodeId id = m_id; m_tree->entry(id).parent != kInvalidNodeId; id = m_tree->entry(id).parent)
    {
        auto const& entry = m_tree->entry(id);
        auto const& parent = m_tree->entry(entry.parent);

        String<24> element;
        StringView segment;
        bool dot = false;
        if (parent.kind == NodeKind::Array)
        {
            size_t index = 0;
            for (NodeId sibling = parent.first_child; sibling != id;
                 sibling = m_tree->entry(sibling).next_sibling)
            {
                ++index;
            }
            format_to(element, "[{}]", static_cast<unsigned long long>(index));
            segment = StringView{element.data(), element.size()};
        }
        else
        {
            segment = StringView{entry.key.data(), entry.key.size()};
            dot = parent.parent != kInvalidNodeId; // members of the root have no leading dot
        }

        if (result.size() + segment.size() + (dot ? 1 : 0) > capacity)
        {
            truncated = true;
            break;
        }
        for (size_t i = segment.size(); i > 0; --i)
        {
            result.push_back(segment[i - 1]);
        }
        if (dot)
        {
            result.push_back('.');
        }
    }

    if (truncated)
    {
        if (!result.empty() && result[result.size() - 1] == '.')
        {
            result.pop_back();
        }
        Private::append(result, kEllipsis);
    }

    for (size_t i = 0, j = result.size(); i + 1 < j; ++i, --j)
    {
        char const c = result[i];
        result[i] = result[j - 1];
        result[j - 1] = c;
    }
    return result;
}

// ========== PropTree ==========

DUCTA_DECL PropTree::PropTree(SourceLocation root_location)
{
    clear(root_location);
}

DUCTA_DECL void PropTree::clear(SourceLocation root_location)
{
    m_entries.clear();
    m_source_name.clear();
    Entry root;
    root.kind = NodeKind::Object;
    root.line = root_location.line();
    root.column = root_location.column();
    m_entries.push_back(::ducta::move(root));
}

DUCTA_DECL void PropTree::set_source_name(StringView name)
{
    size_t const max_size = m_source_name.max_size();
    if (name.size() > max_size)
    {
        name = name.substr(name.size() - max_size);
    }
    m_source_name.assign(name.data(), name.size());
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_entry(
    NodeId parent,
    StringView key,
    NodeKind kind,
    SourceLocation location)
{
    if (parent >= m_entries.size())
    {
        return Unexpected<Error>{Private::make_error("invalid parent node id {}", parent)};
    }

    Entry const& parent_entry = m_entries[parent];
    if (parent_entry.kind == NodeKind::Object)
    {
        if (key.empty())
        {
            return Unexpected<Error>{Private::make_error("object member requires a key")};
        }
        if (PropNode{*this, parent}.child(key).valid())
        {
            return Unexpected<Error>{Private::make_error(
                "duplicate key '{}' at {}:{}", key, location.line(), location.column())};
        }
    }
    else if (parent_entry.kind == NodeKind::Array)
    {
        if (!key.empty())
        {
            return Unexpected<Error>{Private::make_error("array element must not have a key")};
        }
    }
    else
    {
        return Unexpected<Error>{
            Private::make_error("cannot add a child to a {} value", to_string(parent_entry.kind))};
    }

    if (m_entries.size() >= Private::kMaxNodes)
    {
        return Unexpected<Error>{Private::make_error(
            "too many configuration nodes (max {})",
            static_cast<unsigned long long>(Private::kMaxNodes))};
    }

    Entry entry;
    if (key.size() > entry.key.max_size())
    {
        return Unexpected<Error>{Private::make_error(
            "key is too long (max {} characters)",
            static_cast<unsigned long long>(entry.key.max_size()))};
    }
    entry.key.assign(key.data(), key.size());
    entry.kind = kind;
    entry.line = location.line();
    entry.column = location.column();
    entry.parent = parent;

    NodeId id = static_cast<NodeId>(m_entries.size());
    m_entries.push_back(::ducta::move(entry));

    // Link after push_back: it may reallocate and invalidate references
    Entry& parent_ref = m_entries[parent];
    if (parent_ref.last_child == kInvalidNodeId)
    {
        parent_ref.first_child = id;
    }
    else
    {
        m_entries[parent_ref.last_child].next_sibling = id;
    }
    parent_ref.last_child = id;
    ++parent_ref.child_count;
    return id;
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_object(NodeId parent, StringView key, SourceLocation location)
{
    return add_entry(parent, key, NodeKind::Object, location);
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_array(NodeId parent, StringView key, SourceLocation location)
{
    return add_entry(parent, key, NodeKind::Array, location);
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_null(NodeId parent, StringView key, SourceLocation location)
{
    return add_entry(parent, key, NodeKind::Null, location);
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_bool(
    NodeId parent,
    StringView key,
    bool value,
    SourceLocation location)
{
    auto id = add_entry(parent, key, NodeKind::Bool, location);
    if (id)
    {
        m_entries[*id].bool_value = value;
    }
    return id;
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_integer(
    NodeId parent,
    StringView key,
    int64_t value,
    SourceLocation location)
{
    auto id = add_entry(parent, key, NodeKind::Integer, location);
    if (id)
    {
        m_entries[*id].integer_value = value;
    }
    return id;
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_float(
    NodeId parent,
    StringView key,
    double value,
    SourceLocation location)
{
    auto id = add_entry(parent, key, NodeKind::Float, location);
    if (id)
    {
        m_entries[*id].float_value = value;
    }
    return id;
}

DUCTA_DECL Expected<NodeId, Error> PropTree::add_string(
    NodeId parent,
    StringView key,
    StringView value,
    SourceLocation location)
{
    if (value.size() > StringValue{}.max_size())
    {
        return Unexpected<Error>{Private::make_error(
            "string value is too long (max {} characters) at {}:{}",
            static_cast<unsigned long long>(StringValue{}.max_size()),
            location.line(),
            location.column())};
    }
    auto id = add_entry(parent, key, NodeKind::String, location);
    if (id)
    {
        m_entries[*id].string_value.assign(value.data(), value.size());
    }
    return id;
}

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_PRIVATE_PROP_TREE_INL
