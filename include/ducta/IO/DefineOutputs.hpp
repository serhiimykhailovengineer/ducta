/**
 * @file DefineOutputs.hpp
 * @brief Macros to define node outputs
 */

#ifndef DUCTA_DEFINE_OUTPUTS_HPP
#define DUCTA_DEFINE_OUTPUTS_HPP

#include "ducta/IO/OutputRef.hpp"

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <map>


// 1) One entry: ("name", member) -> { "name", OutputRef{ node.member } }
#define DUCTA_NODE_OUTPUTS_ELEM(r, node, i, elem)                                           \
    BOOST_PP_COMMA_IF(i)                                                                    \
    {                                                                                       \
        BOOST_PP_TUPLE_ELEM(2, 0, elem),                                                    \
        ::ducta::IO::OutputRef{ node.BOOST_PP_TUPLE_ELEM(2, 1, elem) }                      \
    }

#define DUCTA_NODE_OUTPUTS_INIT(node, ...)                                                  \
    BOOST_PP_SEQ_FOR_EACH_I(                                                                \
        DUCTA_NODE_OUTPUTS_ELEM,                                                            \
        node,                                                                               \
        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                                               \
    )

// 3) Main macro
#define DEFINE_NODE_OUTPUTS(NodeType, ...)                                              \
template<>                                                                              \
struct ::ducta::IO::NodeOutputsTraits<NodeType>                                         \
{                                                                                       \
    using Node = NodeType;                                                              \
    static std::map<std::string, ::ducta::IO::OutputRef> get(Node& node)                \
    {                                                                                   \
        return std::map<std::string, ::ducta::IO::OutputRef>{                           \
            DUCTA_NODE_OUTPUTS_INIT(node, __VA_ARGS__)                                  \
        };                                                                              \
    }                                                                                   \
};

#endif // DUCTA_DEFINE_OUTPUTS_HPP