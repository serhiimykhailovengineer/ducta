#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/InputRef.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(InputRefTests, value_input) 
{
    IO::TInput<int> input{};

    IO::InputRef input_ref{input};
    EXPECT_FALSE(input_ref.ready());
}

TEST(InputRefTests, copy_ref) 
{
    IO::TInput<int> input{};

    IO::InputRef input_ref{input};
    
    IO::InputRef input_another_ref{input_ref};
}