#include <gtest/gtest.h>

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

#include "ducta/IO/TInputMock.hpp"

#include "ducta/IO/InputRef.hpp"

using namespace ducta;

TEST(TOutputTests, types_check) 
{
    {
        static_assert(std::is_same_v<IO::TOutput<int>::value_type, int>);
        static_assert(std::is_same_v<IO::TOutput<int>::storage_type, int>);
    }

    {
        static_assert(std::is_same_v<IO::TOutput<int&>::value_type, int>);
        static_assert(std::is_same_v<IO::TOutput<int&>::storage_type, int>);
    }

    {
        static_assert(std::is_same_v<IO::TOutput<const float>::value_type, float>);
        static_assert(std::is_same_v<IO::TOutput<const float>::storage_type, float>);
    }

    {
        static_assert(std::is_same_v<IO::TOutput<const float&>::value_type, float>);
        static_assert(std::is_same_v<IO::TOutput<const float&>::storage_type, float>);
    }
}

TEST(TOutputTests, set_const_ref) 
{
    IO::TOutput<int> output{};

    int temp_value = 100;
    EXPECT_NO_THROW(output.set(temp_value));

    EXPECT_EQ(output.type_id(), Utils::type_id<int>());
}

TEST(TOutputTests, set_move) 
{
    IO::TOutput<int> output{};

    int temp_value = 200;
    EXPECT_NO_THROW(output.set(std::move(temp_value)));

    EXPECT_EQ(output.type_id(), Utils::type_id<int>());
}

TEST(TOutputTests, assign_operator) 
{
    IO::TOutput<int> output{};

    int temp_value = 300;
    output = temp_value;

    temp_value = 400;
    EXPECT_NO_THROW(output = std::move(temp_value));
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