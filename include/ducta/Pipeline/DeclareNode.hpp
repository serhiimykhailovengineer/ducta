/**
 * @file DeclareNode.hpp
 * @brief Macro registering a node type in the global NodeFactory
 */

#ifndef DUCTA_PIPELINE_DECLARE_NODE_HPP
#define DUCTA_PIPELINE_DECLARE_NODE_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Preprocessor.hpp"
#include "ducta/Pipeline/NodeFactory.hpp"

// Registers NodeClassName under the name NodeName in NodeFactory::instance():
//   DECLARE_NODE(Gain, my::GainNode);
//   DECLARE_NODE(IntAdder, my::Adder<int>);          // template arguments may contain commas
// The registered function constructs the node from its configuration: a node declaring
// `using Config = ...` gets the decoded and validated Config in its constructor
// (NodeClassName(Config const&)), other nodes are value-initialised.
//
// Like DEFINE_CONFIG, the macro must be used at global scope: it defines the registrar
// ducta::Pipeline::DeclaredNodes::registrar_<NodeName>, so every NodeName is unique in the
// program (a duplicate in one translation unit does not compile). NodeClassName is looked up
// from inside that namespace, so qualify it fully (my::GainNode, or ::GainNode for a type in
// the global namespace whose name may clash with a ducta name).
//
// The macro may also be used in headers (the registrar is an inline variable, so the type is
// registered once per program). Registration happens during static initialisation: when
// DECLARE_NODE is in a source file of a static library, make sure that object file is linked
// (the linker drops unreferenced objects).
// clang-format off
#define DECLARE_NODE(NodeName, ...)                                                          \
    namespace ducta {                                                                        \
    namespace Pipeline {                                                                     \
    namespace DeclaredNodes {                                                                \
    [[maybe_unused]] inline ::ducta::Pipeline::Private::NodeRegistrar<__VA_ARGS__> const     \
        DUCTA_PP_CAT(registrar_, NodeName){#NodeName};                                       \
    }                                                                                        \
    }                                                                                        \
    }                                                                                        \
    static_assert(sizeof(#NodeName) > 1, "DECLARE_NODE: node name must not be empty")
// clang-format on

#endif // DUCTA_PIPELINE_DECLARE_NODE_HPP
