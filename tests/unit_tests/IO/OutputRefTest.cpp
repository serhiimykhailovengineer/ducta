#include <gtest/gtest.h>

#include "ducta/IO/TInputMock.hpp"
#include "ducta/IO/InputRef.hpp"

#include "ducta/IO/TOutputMock.hpp"
#include "ducta/IO/OutputRef.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(OutputRefTests, value_bind_check) 
{
    IO::TOutputMock<int> output_mock{};
    
    IO::OutputRef output_ref{output_mock};

    IO::TInputMock<int> input{};

    IO::InputRef input_ref{input};

    EXPECT_CALL(output_mock, bind(input_ref)).Times(1)
        .WillOnce(testing::Return(Connection{}));

    auto deferred = bind(output_ref, input_ref);
}

TEST(OutputRefTests, copy_ref) 
{
    IO::TOutputMock<int> output_mock{};

    IO::OutputRef output_ref{output_mock};
    
    IO::OutputRef output_another_ref{output_ref};

    EXPECT_NE(&output_ref, &output_another_ref);
    EXPECT_EQ(output_ref, output_another_ref);
}