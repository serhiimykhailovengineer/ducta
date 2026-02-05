/**
 * @file DefineInputs.hpp
 * @brief Macros to define node inputs
 */

#ifndef DUCTA_DEFINE_INPUTS_HPP
#define DUCTA_DEFINE_INPUTS_HPP

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/IO/InputRef.hpp"

#include "ducta/Core/Preprocessor.hpp"

#define DUCTA_NODE_INPUTS_ELEM(node, i, elem)                                   \
    DUCTA_COMMA_IF(i)                                                           \
    ::ducta::pair<::ducta::StringView, ::ducta::IO::InputRef>{                  \
        DUCTA_TUPLE_ELEM(2, 0, elem),                                           \
        ::ducta::IO::InputRef{ (node).DUCTA_TUPLE_ELEM(2, 1, elem) }            \
    }

#define DUCTA_NODE_INPUTS_INIT(node, ...)                                       \
    DUCTA_FOR_EACH_I(DUCTA_NODE_INPUTS_ELEM, node, __VA_ARGS__)

// Main macro to define inputs for a node type
#define DEFINE_NODE_INPUTS(NodeType, ...)                                                 \
template<>                                                                                \
struct ducta::IO::NodeInputsTraits<DUCTA_PP_UNPAREN((NodeType))>                          \
{                                                                                         \
    using Node = DUCTA_PP_UNPAREN((NodeType));                                            \
    static ::ducta::Map<::ducta::StringView, ::ducta::IO::InputRef, 25> get(Node& node)   \
    {                                                                                     \
        return ::ducta::makeMap<::ducta::StringView, ::ducta::IO::InputRef, 25>           \
        (                                                                                 \
            DUCTA_NODE_INPUTS_INIT(node, __VA_ARGS__)                                     \
        );                                                                                \
    }                                                                                     \
};

#endif // DUCTA_DEFINE_INPUTS_HPP