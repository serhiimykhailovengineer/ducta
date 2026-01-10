#include <gtest/gtest.h>

#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TOutput.hpp"

struct TestNode
{
    ducta::IO::TOutput<int> output1;
    ducta::IO::TOutput<float> output2;
};

DEFINE_NODE_OUTPUTS(TestNode, 
        ("output1", output1),
        ("output2", output2)
    );

TEST(DefineOutputsTests, base_test) 
{
    TestNode node;

    auto outputs = ducta::IO::NodeOutputsTraits<TestNode>::get(node);
    EXPECT_EQ(outputs.size(), 2);
    EXPECT_TRUE(outputs.find("output1") != outputs.end());
    EXPECT_TRUE(outputs.find("output2") != outputs.end());
}