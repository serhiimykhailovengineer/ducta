/**
 * @file DefineOutputs.hpp
 * @brief Macros to define node outputs
 */

#ifndef DUCTA_DEFINE_OUTPUTS_HPP
#define DUCTA_DEFINE_OUTPUTS_HPP

#include "ducta/IO/OutputRef.hpp"

#include "ducta/Core/Preprocessor.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"


// 1) One entry: ("name", member) -> { "name", OutputRef{ node.member } }
#define DUCTA_NODE_OUTPUTS_ELEM(node, i, elem)                                   \
    DUCTA_COMMA_IF(i)                                                            \
    ::ducta::pair<::ducta::StringView, ::ducta::IO::OutputRef>{                  \
        DUCTA_TUPLE_ELEM(2, 0, elem),                                            \
        ::ducta::IO::OutputRef{ (node).DUCTA_TUPLE_ELEM(2, 1, elem) }            \
    }

#define DUCTA_NODE_OUTPUTS_INIT(node, ...)                                       \
    DUCTA_FOR_EACH_I(DUCTA_NODE_OUTPUTS_ELEM, node, __VA_ARGS__)


// 3) Main macro to define outputs for a node type
#define DEFINE_NODE_OUTPUTS(NodeType, ...)                                                 \
template<>                                                                                 \
struct ducta::IO::NodeOutputsTraits<DUCTA_PP_UNPAREN((NodeType))>                          \
{                                                                                          \
    using Node = DUCTA_PP_UNPAREN((NodeType));                                             \
    static ::ducta::Map<::ducta::StringView, ::ducta::IO::OutputRef, 25> get(Node& node)   \
    {                                                                                      \
        return ::ducta::makeMap<::ducta::StringView, ::ducta::IO::OutputRef, 25>           \
        (                                                                                  \
            DUCTA_NODE_OUTPUTS_INIT(node, __VA_ARGS__)                                     \
        );                                                                                 \
    }                                                                                      \
};

#endif // DUCTA_DEFINE_OUTPUTS_HPP