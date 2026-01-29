#ifndef DUCTA_PIPELINE_NODE_HPP
#define DUCTA_PIPELINE_NODE_HPP

#include <map>
#include <memory>

#include "ducta/Pipeline/Private/NodeModel.hpp"

namespace ducta {
namespace Pipeline {

class Node
{
    template <typename NodeType>
    std::unique_ptr<Private::NodeConcept> make_node_concept(NodeType&& node)
    {
        using decayed_node_type = std::decay_t<NodeType>;
        return std::make_unique<Private::NodeModel<decayed_node_type>>(std::forward<NodeType>(node));
    }

    template <typename NodeType, typename... Args>
    std::unique_ptr<Private::NodeConcept> make_node_concept(std::in_place_type_t<NodeType>, Args&&... args)
    {
        using decayed_node_type = std::decay_t<NodeType>;
        return std::make_unique<Private::NodeModel<decayed_node_type>>(std::in_place_t{}, std::forward<Args>(args)...);
    }
    
public:
    /*
    * @brief Construct a Node from a concrete node type
    * @tparam NodeType The concrete node type
    * @param node The node object to wrap
    */
    template <typename NodeType>
    Node(NodeType&& node)
    : m_concept{make_node_concept(std::forward<NodeType>(node))}
    {
    }

    /*
    * @brief Construct a Node in-place from its type and constructor arguments
    * @tparam NodeType The concrete node type
    * @tparam Args The types of the constructor arguments
    * @param in_placer Tag to indicate in-place construction
    * @param args The constructor arguments
    */
    template <typename NodeType, typename... Args>
    Node(std::in_place_type_t<NodeType> in_placer, Args&&... args)
    : m_concept{make_node_concept(in_placer, std::forward<Args>(args)...)}
    {}
    
    /*
     * @brief Initialize the node
    */
    void init();

    /*
     * @brief Perform an iteration of the node
     * @return Utils::Expected<bool, Error>: true if iteration was successful and should continue iteration, 
     *                                       false if iteration should stop, or an Error if iteration failed
    */
    Utils::Expected<bool, Utils::Error> iterate();

    /*
     * @brief Release the node
    */
    void release();

    /*
     * @brief Get the inputs of the node
     * @return A map of input names to InputRef objects
    */
    std::map<std::string, IO::InputRef> get_inputs();

    /*
     * @brief Get the outputs of the node
     * @return A map of output names to OutputRef objects
    */
    std::map<std::string, IO::OutputRef> get_outputs();

private:
    std::unique_ptr<Private::NodeConcept> m_concept;
};

template <typename NodeType, typename... Args>
Node make_node(Args&&... args)
{
    return Node(std::in_place_type_t<NodeType>{}, std::forward<Args>(args)...);
}

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_HPP