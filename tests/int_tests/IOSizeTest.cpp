#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"


using namespace ducta;

class Input
{
    std::optional<int> value;
};

TEST(IOSizeTest, int_size) 
{
    EXPECT_EQ(sizeof(int), 4);
    EXPECT_EQ(sizeof(Input), 8);

    EXPECT_EQ(sizeof(ducta::IO::TInput<int>), 8);
    EXPECT_EQ(sizeof(ducta::IO::TInput<int&>), 8);
}