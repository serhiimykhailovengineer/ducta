#ifndef DUCTA_PIPELINE_NODES_HPP
#define DUCTA_PIPELINE_NODES_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Capacity.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Utility.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"

namespace ducta {
namespace Pipeline {

using NodeName = ::ducta::String<DUCTA_MAX_NODE_NAME_LENGTH>;

class Nodes
{
public:
    Nodes() = default;

    void add_node(::ducta::StringView name, NodeRef node)
    {
        NodeName node_name{name};
        m_nodes.insert(::ducta::pair<NodeName, NodeRef>(::ducta::move(node_name), ::ducta::move(node)));
    }

    template <class NodeType>
    void add_node(::ducta::StringView name, NodeType& node)
    {
        NodeName node_name{name};
        m_nodes.insert(::ducta::pair<NodeName, NodeRef>(::ducta::move(node_name), NodeRef(node)));
    }

    Optional<NodeInfo> get_node(::ducta::StringView name)
    {
        auto it = m_nodes.find(NodeName{name});
        if (it != m_nodes.end())
        {
            return Optional<NodeInfo>{NodeInfo{it->first, it->second}};
        }
        return {};
    }

private:
    ::ducta::Map<NodeName, NodeRef, DUCTA_MAX_NODES> m_nodes;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODES_HPP