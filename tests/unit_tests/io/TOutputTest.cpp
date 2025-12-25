#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"

#include "ducta/io/InputRef.hpp"

using namespace ducta;

TEST(TOutputTests, set_const_ref) 
{
    IO::TOutput<int> output{};

    int temp_value = 100;
    output.set(temp_value);
}

TEST(TOutputTests, set_move) 
{
    IO::TOutput<int> output{};

    int temp_value = 200;
    output.set(std::move(temp_value));
}

TEST(TOutputTests, assign_operator) 
{
    IO::TOutput<int> output{};

    int temp_value = 300;
    output = temp_value;

    temp_value = 400;
    output = std::move(temp_value);
}

TEST(TOutputTests, bind) 
{
    IO::TOutput<int> output{};

    IO::TInput<int> input{};
    IO::InputRef input_ref{input};

    {
        auto deferred = output.bind(input_ref);

        EXPECT_FALSE(input.ready());
        
        int test_value = 500;
        output.set(test_value);

        ASSERT_TRUE(input.ready());
        EXPECT_EQ(input.value(), 500);
        EXPECT_FALSE(input.ready());
    }
    
    output.set(400);
    EXPECT_FALSE(input.ready());
}