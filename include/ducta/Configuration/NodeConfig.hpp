#ifndef DUCTA_CONFIGURATION_NODE_CONFIG_HPP
#define DUCTA_CONFIGURATION_NODE_CONFIG_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Optional.hpp"

namespace ducta {
namespace Configuration {

/*
 * @brief A node is configurable when it declares the struct it is configured with:
 *        struct Gain { using Config = GainConfig; ... };
 */
template <class NodeType, class = void>
struct has_config : ::ducta::false_type
{
};

template <class NodeType>
struct has_config<NodeType, ::ducta::void_t<typename NodeType::Config>> : ::ducta::true_type
{
};

template <class NodeType>
constexpr bool has_config_v = has_config<NodeType>::value;

template <class NodeType>
using config_t = typename NodeType::Config;

/*
 * @brief Decode the configuration of NodeType from node
 * @return The configuration, or an empty Optional if diagnostics reported any problem
 */
template <class NodeType>
Optional<config_t<NodeType>> decode_node_config(PropNode node, Diagnostics& diagnostics)
{
    static_assert(has_config_v<NodeType>, "Node type does not declare 'using Config = ...'");
    return decode<config_t<NodeType>>(node, diagnostics);
}

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_CONFIGURATION_NODE_CONFIG_HPP
