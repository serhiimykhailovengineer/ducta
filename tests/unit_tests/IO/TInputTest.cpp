#include <gtest/gtest.h>

#include "ducta/IO/TInput.hpp"

#include "ducta/IO/TInputMock.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(TInputTests, types_check) 
{
    {
        static_assert(std::is_same_v<IO::TInput<int>::value_type, int>);
        static_assert(std::is_same_v<IO::TInput<int>::storage_type, int>);
    }

    {
        static_assert(std::is_same_v<IO::TInput<int&>::value_type, int>);
        static_assert(std::is_same_v<IO::TInput<int&>::storage_type, const int&>);
    }

    {
        static_assert(std::is_same_v<IO::TInput<const float>::value_type, float>);
        static_assert(std::is_same_v<IO::TInput<const float>::storage_type, float>);
    }

    {
        static_assert(std::is_same_v<IO::TInput<const float&>::value_type, float>);
        static_assert(std::is_same_v<IO::TInput<const float&>::storage_type, const float&>);
    }
}

TEST(TInputTests, value_input) 
{
    int input_value = 10;
    IO::TInput<int> input{};

    EXPECT_FALSE(input.ready());
    EXPECT_TRUE(input.compatible(::ducta::type_id<int>()));
    EXPECT_TRUE(input.compatible(::ducta::type_id<int&>()));
    EXPECT_FALSE(input.compatible(::ducta::type_id<float>()));
    EXPECT_THROW(input.value(), std::runtime_error);
    EXPECT_EQ(input.value_or(input_value), input_value);

    input_value = 42;

    input.notify(input_value);

    EXPECT_TRUE(input.ready());
    EXPECT_TRUE(static_cast<bool>(input));
    {
        int const& val = input.value();
        EXPECT_EQ(val, 42);
        EXPECT_FALSE(input.ready());
        input_value = 100;
        EXPECT_EQ(val, 42);
    }
}

TEST(TInputTests, ref_input) 
{
    int input_value = 10;
    IO::TInput<int&> input{};

    EXPECT_FALSE(input.ready());
    EXPECT_TRUE(input.compatible(::ducta::type_id<int>()));
    EXPECT_TRUE(input.compatible(::ducta::type_id<int&>()));
    EXPECT_FALSE(input.compatible(::ducta::type_id<float>()));
    EXPECT_THROW(input.value(), std::runtime_error);
    EXPECT_EQ(input.value_or(input_value), input_value);

    input_value = 42;

    input.notify(input_value);

    EXPECT_TRUE(input.ready());
    EXPECT_TRUE(static_cast<bool>(input));
    {
        int const& val = input.value();
        EXPECT_EQ(val, 42);
        EXPECT_FALSE(input.ready());
        input_value = 100;
        EXPECT_EQ(val, 100);
    }
}
