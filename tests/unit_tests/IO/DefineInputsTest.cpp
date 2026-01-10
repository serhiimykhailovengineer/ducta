#include <gtest/gtest.h>

#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/TInput.hpp"

struct TestNode
{
    ducta::IO::TInput<int> input1;
    ducta::IO::TInput<float> input2;
};

DEFINE_NODE_INPUTS(TestNode, 
        ("input1", input1),
        ("input2", input2)
    );

TEST(DefineInputsTests, base_test) 
{
    TestNode node;

    auto inputs = ducta::IO::NodeInputsTraits<TestNode>::get(node);
    EXPECT_EQ(inputs.size(), 2);
    EXPECT_TRUE(inputs.find("input1") != inputs.end());
    EXPECT_TRUE(inputs.find("input2") != inputs.end());
}