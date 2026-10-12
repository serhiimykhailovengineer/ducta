#include <gtest/gtest.h>

#include "ducta/Pipeline/NodeFactory.hpp"

#include "TestNodes.hpp"

using namespace ducta;
using namespace ducta::Pipeline;

TEST(NodeFactoryTest, declare_node_registers_in_global_factory)
{
    // Registrars live in the library namespace
    (void)Pipeline::DeclaredNodes::registrar_Gain;
    (void)Pipeline::DeclaredNodes::registrar_IntFloatPair;

    NodeFactory const& factory = NodeFactory::instance();
    EXPECT_TRUE(factory.contains("Gain"));
    EXPECT_TRUE(factory.contains("Plain"));
    EXPECT_TRUE(factory.contains("IntFloatPair"));
    EXPECT_FALSE(factory.contains("Nope"));

    EXPECT_EQ(factory.find("Gain"), &Pipeline::Private::create_node<TestNodes::GainNode>);
    EXPECT_EQ(factory.find("Nope"), nullptr);
}

TEST(NodeFactoryTest, separate_factory_instance)
{
    NodeFactory factory;
    factory.register_type<TestNodes::PlainNode>("Plain");

    EXPECT_EQ(factory.size(), 1u);
    EXPECT_TRUE(factory.contains("Plain"));
    EXPECT_FALSE(factory.contains("Gain"));
}

TEST(NodeFactoryTest, duplicate_name_keeps_first_type)
{
    NodeFactory factory;
    factory.register_type<TestNodes::PlainNode>("Node");
    factory.register_type<TestNodes::GainNode>("Node");

    EXPECT_EQ(factory.size(), 1u);
    EXPECT_EQ(factory.find("Node"), &Pipeline::Private::create_node<TestNodes::PlainNode>);
}
