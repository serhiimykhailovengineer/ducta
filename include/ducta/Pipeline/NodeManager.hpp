/**
 * @file NodeManager.hpp
 * @brief Creates and owns the nodes of a pipeline described by a configuration
 */

#ifndef DUCTA_PIPELINE_NODE_MANAGER_HPP
#define DUCTA_PIPELINE_NODE_MANAGER_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/DefineConfig.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Pipeline/NodeFactory.hpp"
#include "ducta/Pipeline/NodeStorage.hpp"
#include "ducta/Pipeline/Nodes.hpp"

namespace ducta {
namespace Pipeline {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using DefaultNodeStorage = StaticNodeStorage<DUCTA_NODE_STORAGE_BYTES, DUCTA_MAX_NODES>;
#else
using DefaultNodeStorage = HeapNodeStorage;
#endif

/*
 * @brief One node to create: its type and its configuration
 *   { "type": "Gain", "config": { "gain": 2.0 } }
 */
struct NodeSpec
{
    /* Type name given to DECLARE_NODE */
    NodeName type;
    /* Configuration of the node; an invalid PropNode (no "config") means defaults */
    Configuration::PropNode config;
};

/*
 * @brief Nodes to create: instance name -> its type and configuration
 * @note A node is registered in Nodes under its instance name, so several instances of one
 *       type can be created, with the same or different configurations.
 */
using NodeConfigs = Map<NodeName, NodeSpec, DUCTA_MAX_NODES>;

/* Decoded from { "nodes": { "left": { "type": "Gain", "config": { "gain": 2.0 } } } } */
struct NodeManagerConfig
{
    NodeConfigs nodes;
};

/*
 * @brief Creates nodes with NodeFactory and keeps them in Storage
 * @tparam Storage Storage policy (see NodeStorage.hpp): StaticNodeStorage<Bytes, MaxNodes>
 *         or HeapNodeStorage
 * @note Nodes live until the next configure(), clear() or the destruction of the manager;
 *       they are destroyed in reverse order of creation.
 */
template <class Storage = DefaultNodeStorage>
class NodeManager
{
public:
    using Config = NodeManagerConfig;

    explicit NodeManager(NodeFactory const& factory = NodeFactory::instance()) noexcept
        : m_factory{&factory}
    {
    }

    NodeManager(NodeManager const&) = delete;
    NodeManager& operator=(NodeManager const&) = delete;

    ~NodeManager()
    {
        clear();
    }

    /*
     * @brief Decode Config from the root of tree, then create the nodes (see below)
     * @note The tree may be discarded afterwards
     */
    bool configure(Configuration::PropTree const& tree, Configuration::Diagnostics& diagnostics)
    {
        clear();
        Config config;
        if (!Configuration::decode(tree.root(), config, diagnostics))
        {
            return false;
        }
        return configure(config, diagnostics);
    }

    /*
     * @brief Replace all nodes by the ones described in config
     *
     * Every problem (unknown node type, configuration error, no room in Storage) is reported to
     * diagnostics. Creation is all or nothing: on any error no node is kept, also when a node
     * constructor throws (the exception is passed on).
     *
     * @return true if every node was created
     */
    bool configure(Config const& config, Configuration::Diagnostics& diagnostics)
    {
        clear();
        size_t const errors = diagnostics.error_count();

        for (auto const& entry : config.nodes)
        {
            StringView const name{entry.first.data(), entry.first.size()};
            NodeSpec const& spec = entry.second;
            StringView const type{spec.type.data(), spec.type.size()};
            CreateNodeFn create = m_factory->find(type);
            if (create == nullptr)
            {
                diagnostics.errorf(spec.config, "unknown node type '{}' of node '{}'", type, name);
                continue;
            }
            Slot slot{*this, name};
#if defined(__cpp_exceptions)
            try
            {
                create(slot, spec.config, diagnostics);
            }
            catch (...)
            {
                // A node constructor threw: keep the all-or-nothing guarantee
                clear();
                throw;
            }
#else
            create(slot, spec.config, diagnostics);
#endif
        }

        if (diagnostics.error_count() != errors)
        {
            clear();
            return false;
        }
        return true;
    }

    /* Destroy all nodes */
    void clear() noexcept
    {
        m_nodes = Nodes{};
        m_storage.clear();
    }

    /* Created nodes by name, e.g. for Pipeline::configure */
    Nodes& nodes() noexcept
    {
        return m_nodes;
    }

    Nodes const& nodes() const noexcept
    {
        return m_nodes;
    }

    /* Number of created nodes */
    size_t size() const noexcept
    {
        return m_storage.size();
    }

    Storage const& storage() const noexcept
    {
        return m_storage;
    }

private:
    /* NodeStorage given to CreateNodeFn: places the node in m_storage and names it */
    class Slot final : public NodeStorage
    {
    public:
        Slot(NodeManager& manager, StringView name) noexcept
            : m_manager{manager}
            , m_name{name}
        {
        }

    private:
        void* allocate(size_t size, size_t align) noexcept override
        {
            return m_manager.m_storage.allocate(size, align);
        }

        void deallocate(void* memory, size_t align) noexcept override
        {
            m_manager.m_storage.deallocate(memory, align);
        }

        void adopt(NodeRef node, void* object, size_t align, NodeDestroyFn destroy) override
        {
            m_manager.m_storage.adopt(object, align, destroy);
            m_manager.m_nodes.add_node(m_name, node);
        }

    private:
        NodeManager& m_manager;
        StringView m_name;
    };

private:
    NodeFactory const* m_factory;
    Storage m_storage;
    Nodes m_nodes;
};

} // namespace Pipeline
} // namespace ducta

DEFINE_CONFIG(ducta::Pipeline::NodeSpec, ("type", type), ("config", config));

template <>
struct ducta::Configuration::ConfigRules<ducta::Pipeline::NodeSpec>
{
    static void validate(ducta::Pipeline::NodeSpec const& /*spec*/, ::ducta::Configuration::Validator& validator)
    {
        validator.require("type");
    }
};

DEFINE_CONFIG(ducta::Pipeline::NodeManagerConfig, ("nodes", nodes));

#endif // DUCTA_PIPELINE_NODE_MANAGER_HPP
