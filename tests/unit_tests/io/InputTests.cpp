#include <gtest/gtest.h>

#include "IO/Input.hpp"

#include "IO/InputMock.hpp"

using ::testing::_;

TEST(InputTests, ready_and_compatible) 
{
    IO::InputMock<int> test_input;
    Pipeline::InputRef inputRef{test_input};

    EXPECT_CALL(test_input, ready())
        .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(test_input, compatible(Utils::type_id<int>()))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(test_input, compatible(Utils::type_id<double>()))
        .WillRepeatedly(testing::Return(false));

    EXPECT_FALSE(inputRef.ready());
    EXPECT_TRUE(inputRef.compatible(Utils::type_id<int>()));
    EXPECT_FALSE(inputRef.compatible(Utils::type_id<double>()));
}

TEST(InputTests, check_value) 
{
    IO::InputMock<int> test_input;
    Pipeline::InputRef inputRef{test_input};

    // EXPECT_CALL(test_input.mock, ready())
    //     .WillOnce(testing::Return(false));
    // EXPECT_CALL(test_input.mock, value())
    //     .Times(0);

    // EXPECT_THROW(test_input.value(), std::runtime_error);
}

// TEST(InputTests, check_value_type_mismatch) 
// {
//     Pipeline::InputMockWrapper<int> test_input;
//     Pipeline::InputRef inputRef{test_input};

//     EXPECT_CALL(test_input.mock, ready())
//         .WillOnce(testing::Return(true));
//     EXPECT_CALL(test_input.mock, compatible(_))
//         .WillOnce(testing::Return(false));
//     EXPECT_CALL(test_input.mock, value())
//         .Times(0);

//     EXPECT_THROW(test_input.value(), std::runtime_error);
// }

TEST(InputTests, check_value_success) 
{
    IO::InputMock<int> test_input;
    Pipeline::InputRef inputRef{test_input};

    int const return_value = 42;

    // EXPECT_CALL(test_input.mock, ready())
    //     .WillOnce(testing::Return(true));
    // EXPECT_CALL(test_input.mock, compatible(Utils::type_id<int>()))
        // .WillOnce(testing::Return(true));
    EXPECT_CALL(test_input, value())
        .WillOnce(testing::ReturnRef(return_value));

    int value = 0;
    EXPECT_NO_THROW(value = test_input.value());
    EXPECT_EQ(value, 42);
}
