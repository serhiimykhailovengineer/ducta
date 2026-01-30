#ifndef DUCTA_PIPELINE_NODES_HPP
#define DUCTA_PIPELINE_NODES_HPP

#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Optional.hpp"

namespace ducta {
namespace Pipeline {

class Nodes
{
public:
    Nodes() = default;

    void add_node(::ducta::StringView name, Node node)
    {
        m_nodes.insert(std::make_pair(name, std::move(node)));
    }

    template <typename... Args>
    void add_node(::ducta::StringView name, Args&&... args)
    {
        m_nodes.insert(std::make_pair(name, Node(std::forward<Args>(args)...)));
    }

    Optional<NodeInfo> get_node(::ducta::StringView name)
    {
        auto it = m_nodes.find(name);
        if (it != m_nodes.end())
        {
            return Optional<NodeInfo>{NodeInfo{name, it->second}};
        }
        return {};
    }
    

private:
    ::ducta::Map<::ducta::StringView, Node, 50> m_nodes;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODES_HPP