/**
 * @file DefineOutputs.hpp
 * @brief Macros to define node outputs
 */

#ifndef DUCTA_DEFINE_OUTPUTS_HPP
#define DUCTA_DEFINE_OUTPUTS_HPP

#include "ducta/IO/OutputRef.hpp"

#include <map>


// 1) One entry: ("name", member) -> { "name", OutputRef{ node.member } }
#define DUCTA_NODE_OUTPUT_PAIR_NODE(name, member) \
    { name, ::ducta::IO::OutputRef{ node.member } }

// 2) FOR_EACH over tuples ( (..), (..), ... )
#define DUCTA_FOR_EACH(macro, ...) \
    DUCTA_FOR_EACH_N(macro, __VA_ARGS__, 8,7,6,5,4,3,2,1)

#define DUCTA_FOR_EACH_N(macro, _1,_2,_3,_4,_5,_6,_7,_8, N, ...) \
    DUCTA_FOR_EACH_##N(macro, _1,_2,_3,_4,_5,_6,_7,_8)

#define DUCTA_FOR_EACH_1(m, a) m a
#define DUCTA_FOR_EACH_2(m, a, b) m a, m b
#define DUCTA_FOR_EACH_3(m, a, b, c) m a, m b, m c
#define DUCTA_FOR_EACH_4(m, a, b, c, d) m a, m b, m c, m d
#define DUCTA_FOR_EACH_5(m, a, b, c, d, e) m a, m b, m c, m d, m e
#define DUCTA_FOR_EACH_6(m, a, b, c, d, e, f) m a, m b, m c, m d, m e, m f
#define DUCTA_FOR_EACH_7(m, a, b, c, d, e, f, g) m a, m b, m c, m d, m e, m f, m g
#define DUCTA_FOR_EACH_8(m, a, b, c, d, e, f, g, h) m a, m b, m c, m d, m e, m f, m g, m h

// 3) Main macro
#define DEFINE_NODE_OUTPUTS(NodeType, ...)                                              \
template<>                                                                              \
struct NodeOutputsTraits<NodeType>                                                      \
{                                                                                       \
    using Node = NodeType;                                                              \
    static std::map<std::string, ::ducta::IO::OutputRef> const& get(Node& node)         \
    {                                                                                   \
        static std::map<std::string, ::ducta::IO::OutputRef> outputs{                   \
            DUCTA_FOR_EACH(DUCTA_NODE_OUTPUT_PAIR_NODE, __VA_ARGS__)                    \
        };                                                                              \
        return outputs;                                                                 \
    }                                                                                   \
};

#endif // DUCTA_DEFINE_OUTPUTS_HPP