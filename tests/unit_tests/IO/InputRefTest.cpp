#include <gtest/gtest.h>

#include "ducta/IO/TInputMock.hpp"
#include "ducta/IO/InputRef.hpp"

using namespace ducta;
using namespace ducta::IO;

TEST(InputRefTests, value_input) 
{
    IO::TInputMock<int> input{};

    EXPECT_CALL(input, ready()).WillOnce(testing::Return(false));
    EXPECT_CALL(input, compatible(::ducta::type_id<int>())).WillOnce(testing::Return(true));
    EXPECT_CALL(input, compatible(::ducta::type_id<float>())).WillOnce(testing::Return(false));
    EXPECT_CALL(input, notify(testing::Eq(42))).Times(1);

    IO::InputRef input_ref{input};
    EXPECT_FALSE(input_ref.ready());
    EXPECT_TRUE(input_ref.compatible(::ducta::type_id<int>()));
    EXPECT_FALSE(input_ref.compatible(::ducta::type_id<float>()));
    EXPECT_NO_THROW(input_ref.notify<int>(42));
}

TEST(InputRefTests, copy_ref) 
{
    IO::TInputMock<int> input{};

    IO::InputRef input_ref{input};
    
    IO::InputRef input_another_ref{input_ref};

    EXPECT_NE(&input_ref, &input_another_ref);
    EXPECT_EQ(input_ref, input_another_ref);
}