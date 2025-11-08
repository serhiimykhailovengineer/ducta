#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"

#include "ducta/io/TOutputMock.hpp"
#include "ducta/io/TInputMock.hpp"

using namespace ducta;

TEST(TOutputTests, set_const_ref) 
{
    IO::TOutputMock<int> output_mock;

    IO::TOutput<int> output{IO::TOutputMockWrapper<int>{output_mock}};

    EXPECT_CALL(output_mock, set_const_ref(testing::_)).Times(1);
    EXPECT_CALL(output_mock, set_move(testing::_)).Times(0);

    int temp_value = 100;
    output.set(temp_value);
}

TEST(TOutputTests, set_move) 
{
    IO::TOutputMock<int> output_mock;

    IO::TOutput<int> output{IO::TOutputMockWrapper<int>{output_mock}};

    EXPECT_CALL(output_mock, set_const_ref(testing::_)).Times(0);
    EXPECT_CALL(output_mock, set_move(testing::_)).Times(1);

    int temp_value = 200;
    output.set(std::move(temp_value));
}

TEST(TOutputTests, assign_operator) 
{
    IO::TOutputMock<int> output_mock;

    IO::TOutput<int> output{IO::TOutputMockWrapper<int>{output_mock}};

    EXPECT_CALL(output_mock, set_const_ref(testing::_)).Times(1);
    EXPECT_CALL(output_mock, set_move(testing::_)).Times(1);

    int temp_value = 300;
    output = temp_value;

    temp_value = 400;
    output = std::move(temp_value);
}

TEST(TOutputTests, bind) 
{
    IO::TOutputMock<int> output_mock;

    IO::TOutput<int> output{IO::TOutputMockWrapper<int>{output_mock}};

    EXPECT_CALL(output_mock, bind(testing::_)).WillOnce(testing::Return(Utils::Deferred{}));

    auto output_ref = output.get_ref();

    IO::TInputMock<int> input_mock;

    IO::TInput<int> input{IO::TInputMockWrapper<int>{input_mock}};

    auto input_ref = input.get_ref();

    auto deferred = bind(output_ref, input_ref);
}