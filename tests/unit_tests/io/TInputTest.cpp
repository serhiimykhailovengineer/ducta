#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"

#include "ducta/io/TInputMock.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(TInputTests, input_ready) 
{
    IO::TInputMock<int> input_mock;

    IO::TInput<int> input{IO::TInputMockWrapper<int>{input_mock}};

    EXPECT_CALL(input_mock, ready())
        .WillOnce(testing::Return(false));

    EXPECT_FALSE(input.ready());
}

TEST(TInputTests, input_ready_and_get) 
{
    IO::TInputMock<int> input_mock;

    IO::TInput<int> input{IO::TInputMockWrapper<int>{input_mock}};

    int ret_value = 42;

    EXPECT_CALL(input_mock, ready())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(input_mock, value())
        .WillOnce(testing::ReturnRef(ret_value));

    EXPECT_TRUE(input.ready());
    EXPECT_EQ(input.value(), 42);
}

// TEST(TInputTests, input_cast) 
// {
//     IO::TInputMock<int> input_mock;

//     IO::TInput<int> input{IO::TInputMockWrapper<int>{input_mock}};

//     auto input_ref = input.get_ref();
//     auto wrapper = cast<IO::TInputMockWrapper<int>>(input_ref);
// }
