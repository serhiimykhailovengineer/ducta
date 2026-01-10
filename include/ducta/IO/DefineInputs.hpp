/**
 * @file DefineInputs.hpp
 * @brief Macros to define node inputs
 */

#ifndef DUCTA_DEFINE_INPUTS_HPP
#define DUCTA_DEFINE_INPUTS_HPP

#include "ducta/IO/InputRef.hpp"

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <map>

// elem == ("input1", input1)
#define DUCTA_NODE_INPUTS_ELEM(r, node, i, elem)                                            \
    BOOST_PP_COMMA_IF(i)                                                                    \
    {                                                                                       \
        BOOST_PP_TUPLE_ELEM(2, 0, elem),                                                    \
        ::ducta::IO::InputRef{ node.BOOST_PP_TUPLE_ELEM(2, 1, elem) }                       \
    }

#define DUCTA_NODE_INPUTS_INIT(node, ...)                                                   \
    BOOST_PP_SEQ_FOR_EACH_I(                                                                \
        DUCTA_NODE_INPUTS_ELEM,                                                             \
        node,                                                                               \
        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                                               \
    )

// 3) Main macro
#define DEFINE_NODE_INPUTS(NodeType, ...)                                               \
template<>                                                                              \
struct ducta::IO::NodeInputsTraits<NodeType>                                            \
{                                                                                       \
    using Node = NodeType;                                                              \
    static std::map<std::string, ::ducta::IO::InputRef> get(Node& node)                 \
    {                                                                                   \
        return std::map<std::string, ::ducta::IO::InputRef>{                            \
            DUCTA_NODE_INPUTS_INIT(node, __VA_ARGS__)                                   \
        };                                                                              \
    }                                                                                   \
};

#endif // DUCTA_DEFINE_INPUTS_HPP