/**
 * @file NodeFactory.hpp
 * @brief Registry of node types that can be created by name from their configuration
 */

#ifndef DUCTA_PIPELINE_NODE_FACTORY_HPP
#define DUCTA_PIPELINE_NODE_FACTORY_HPP

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/NodeConfig.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Utility.hpp"

#include "ducta/Pipeline/NodeStorage.hpp"

// Capacity of the node type registry (number of DECLARE_NODE types)
#ifndef DUCTA_MAX_NODE_TYPES
#define DUCTA_MAX_NODE_TYPES 50
#endif

namespace ducta {
namespace Pipeline {

using NodeTypeName = StringView;

/*
 * @brief Decodes the configuration of one node type and constructs the node in storage
 * @param config Configuration of the node; an invalid handle means "no configuration"
 * @return true if the node was created; false if diagnostics reported an error (nothing is
 *         constructed in that case). Warnings do not prevent creation.
 */
using CreateNodeFn = bool (*)(
    NodeStorage& storage,
    Configuration::PropNode config,
    Configuration::Diagnostics& diagnostics);

namespace Private {

template <class T, class = void>
struct is_config_constructible : ::ducta::false_type
{
};

template <class T>
struct is_config_constructible<
    T,
    ::ducta::void_t<decltype(T(::ducta::declval<Configuration::config_t<T> const&>()))>>
    : ::ducta::true_type
{
};

template <class T, class = void>
struct is_value_constructible : ::ducta::false_type
{
};

template <class T>
struct is_value_constructible<T, ::ducta::void_t<decltype(T())>> : ::ducta::true_type
{
};

template <class T>
bool created(T const* node, Configuration::PropNode config, Configuration::Diagnostics& diagnostics)
{
    if (node == nullptr)
    {
        diagnostics.error(config, "not enough node storage");
        return false;
    }
    return true;
}

/*
 * CreateNodeFn of T. Nodes declaring `using Config = ...` are constructed with the decoded
 * configuration (T(Config const&)); a missing configuration (invalid handle) means a default
 * Config, which still has to pass its ConfigRules. Unknown parameters are reported as warnings
 * and do not prevent creation. Nodes without Config are value-initialised; parameters given to
 * them are ignored with a warning.
 */
template <class T>
bool create_node(NodeStorage& storage, Configuration::PropNode config, Configuration::Diagnostics& diagnostics)
{
    if constexpr (Configuration::has_config_v<T>)
    {
        using Config = Configuration::config_t<T>;
        Config value{};
        bool const ok =
            config.valid()
                ? Configuration::decode(config, value, diagnostics)
                : Configuration::validate(config, static_cast<Config const&>(value), diagnostics);
        if (!ok)
        {
            return false;
        }
        return created(storage.construct<T>(static_cast<Config const&>(value)), config, diagnostics);
    }
    else
    {
        if (config.valid() && !(config.is_object() && config.size() == 0))
        {
            /* Parameters the node does not know about are ignored, like unknown fields */
            diagnostics.warning(config, "node type takes no configuration, parameters are ignored");
        }
        return created(storage.construct<T>(), config, diagnostics);
    }
}

} // namespace Private

/*
 * @brief Maps a node type name (e.g. "Gain" in a configuration) to the function creating it
 * @note Types are usually registered in the global instance() by DECLARE_NODE (DeclareNode.hpp)
 *       during static initialisation; separate instances are useful for tests. Nodes are
 *       created and owned by NodeManager.
 */
class NodeFactory
{
public:
    using Container = Map<NodeTypeName, CreateNodeFn, DUCTA_MAX_NODE_TYPES>;

    NodeFactory() = default;

    static NodeFactory& instance()
    {
        static NodeFactory factory;
        return factory;
    }

    /*
     * @brief Register node type T under name; name must outlive the factory (e.g. a string
     *        literal). A name that is already registered keeps its first type.
     */
    template <class T>
    void register_type(NodeTypeName name)
    {
        using NodeType = ::ducta::decay_t<T>;
        if constexpr (Configuration::has_config_v<NodeType>)
        {
            static_assert(
                Private::is_config_constructible<NodeType>::value,
                "Node type declaring 'using Config = ...' must be constructible from 'Config "
                "const&'");
        }
        else
        {
            static_assert(
                Private::is_value_constructible<NodeType>::value,
                "Node type without 'using Config = ...' must be default constructible");
        }
        m_types.insert(
            ::ducta::pair<NodeTypeName, CreateNodeFn>(name, &Private::create_node<NodeType>));
    }

    /* Creating function of the type, or nullptr if the name is unknown */
    CreateNodeFn find(NodeTypeName name) const
    {
        auto it = m_types.find(name);
        return it != m_types.end() ? it->second : nullptr;
    }

    bool contains(NodeTypeName name) const
    {
        return find(name) != nullptr;
    }

    size_t size() const noexcept
    {
        return m_types.size();
    }

    auto begin() const noexcept
    {
        return m_types.begin();
    }

    auto end() const noexcept
    {
        return m_types.end();
    }

private:
    Container m_types;
};

namespace Private {

/* Registers T in NodeFactory::instance() when constructed (used by DECLARE_NODE) */
template <class T>
struct NodeRegistrar
{
    explicit NodeRegistrar(NodeTypeName name)
    {
        NodeFactory::instance().register_type<T>(name);
    }
};

} // namespace Private

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_FACTORY_HPP
