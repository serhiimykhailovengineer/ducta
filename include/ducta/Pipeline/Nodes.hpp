#ifndef DUCTA_PIPELINE_NODES_HPP
#define DUCTA_PIPELINE_NODES_HPP

#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {
namespace Pipeline {

using NodeName = ::ducta::String<50>;

class Nodes
{
public:
    Nodes() = default;

    void add_node(::ducta::StringView name, Node node)
    {
        NodeName node_name{name};
        m_nodes.insert(::ducta::pair<NodeName, Node>(::ducta::move(node_name), ::ducta::move(node)));
    }

    template <typename... Args>
    void add_node(::ducta::StringView name, Args&&... args)
    {
        NodeName node_name{name};
        m_nodes.insert(::ducta::pair<NodeName, Node>(::ducta::move(node_name), Node(::ducta::forward<Args>(args)...)));
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
    ::ducta::Map<NodeName, Node, 50> m_nodes;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODES_HPP