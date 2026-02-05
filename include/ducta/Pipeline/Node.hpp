#ifndef DUCTA_PIPELINE_NODE_HPP
#define DUCTA_PIPELINE_NODE_HPP

#include <memory>

#include "ducta/Pipeline/Private/NodeModel.hpp"

#include "ducta/Core/TypeTraits.hpp"

namespace ducta {
namespace Pipeline {

class Node
{
    template <typename NodeType>
    std::unique_ptr<Private::NodeConcept> make_node_concept(NodeType&& node) noexcept
    {
        using decayed_node_type = ::ducta::decay_t<NodeType>;
        return std::make_unique<Private::NodeModel<decayed_node_type>>(::ducta::forward<NodeType>(node));
    }

    template <typename NodeType, typename... Args>
    std::unique_ptr<Private::NodeConcept> make_node_concept(::ducta::in_place_type_t<NodeType>, Args&&... args) noexcept
    {
        using decayed_node_type = ::ducta::decay_t<NodeType>;
        return std::make_unique<Private::NodeModel<decayed_node_type>>(::ducta::in_place_t{}, ::ducta::forward<Args>(args)...);
    }
    
public:
    /*
    * @brief Construct a Node from a concrete node type
    * @tparam NodeType The concrete node type
    * @param node The node object to wrap
    * @note This constructor is disabled when NodeType is Node to prevent wrapping a Node in a Node
    */
    template <typename NodeType, 
              typename = typename ::ducta::enable_if<!::ducta::is_same<typename ::ducta::decay<NodeType>::type, Node>::value>::type>
    Node(NodeType&& node) noexcept
    : m_concept{make_node_concept(::ducta::forward<NodeType>(node))}
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
    Node(::ducta::in_place_type_t<NodeType> in_placer, Args&&... args) noexcept
    : m_concept{make_node_concept(in_placer, ::ducta::forward<Args>(args)...)}
    {}

    Node(Node&& other) noexcept
    : m_concept{::ducta::move(other.m_concept)}
    {
    }

    
    /*
     * @brief Initialize the node
    */
    void init();

    /*
     * @brief Perform an iteration of the node
     * @return Expected<bool, Error>: true if iteration was successful and should continue iteration, 
     *                                       false if iteration should stop, or an Error if iteration failed
    */
    Expected<bool, Error> iterate();

    /*
     * @brief Release the node
    */
    void release();

    /*
     * @brief Get the inputs of the node
     * @return A map of input names to InputRef objects
    */
    Map<StringView, IO::InputRef, 25> get_inputs();

    /*
     * @brief Get the outputs of the node
     * @return A map of output names to OutputRef objects
    */
    Map<StringView, IO::OutputRef, 25> get_outputs();

private:
    std::unique_ptr<Private::NodeConcept> m_concept;
};

template <typename NodeType, typename... Args>
Node make_node(Args&&... args)
{
    return Node(::ducta::in_place_type_t<NodeType>{}, ::ducta::forward<Args>(args)...);
}

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_NODE_HPP