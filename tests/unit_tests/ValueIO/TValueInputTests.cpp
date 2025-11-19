#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/ValueIO/TValueInput.hpp"
#include "ducta/ValueIO/TValueOutput.hpp"

using namespace ducta;

TEST(TValueInputTests, input_not_ready) 
{
    IO::TInput<int> input{IO::TValueInput<int>{}};

    EXPECT_FALSE(input.ready());

    EXPECT_THROW(input.value(), std::runtime_error);

    EXPECT_EQ(input.value_or(42), 42);
}

TEST(TValueInputTests, input_ready_and_get) 
{
    int test_value = 123;
    IO::TInput<int> input{std::in_place_type_t<IO::TValueInput<int>>{}, test_value};

    EXPECT_TRUE(input.ready());
    EXPECT_EQ(input.value(), 123);

}

// TEST(ValueIOTests, bind_output_to_input) 
// {
//     Pipeline::ValueInput<int> input;

//     Pipeline::ValueOutput<int> output;

//     {
//         auto deferred = bind(output, input);

//         EXPECT_TRUE(input.ready());

//         output.set(42);

//         EXPECT_TRUE(input.ready());
//         EXPECT_EQ(input.value(), 42);
//     }

//     EXPECT_FALSE(input.ready());

//     output.set(100);

//     EXPECT_FALSE(input.ready());
//     EXPECT_THROW(input.value(), std::runtime_error);
// }
