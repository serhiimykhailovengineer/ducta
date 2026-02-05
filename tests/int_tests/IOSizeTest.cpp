#include <gtest/gtest.h>

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

#include "ducta/Core/Types/ReferenceWrapper.hpp"


using namespace ducta;

class Input
{
    std::optional<int> value;
};

class InputRef
{
    explicit InputRef(Input& input)
    : ref(input)
    {}

    Input& ref;
};

class InputRefWrapper
{
    ::ducta::reference_wrapper<Input> ref;
};

TEST(IOSizeTest, int_size) 
{
    EXPECT_EQ(sizeof(int), 4);
    EXPECT_EQ(sizeof(Input), 8);
    EXPECT_EQ(sizeof(InputRef), 8);
    EXPECT_EQ(sizeof(InputRefWrapper), 8);

    EXPECT_EQ(sizeof(ducta::IO::TInput<int>), 8);
    EXPECT_EQ(sizeof(ducta::IO::TInput<int&>), 8);
}