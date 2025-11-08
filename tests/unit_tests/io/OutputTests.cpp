#include <gtest/gtest.h>


#include "IO/Output.hpp"
#include "IO/OutputMock.hpp"
#include "IO/InputMock.hpp"

using namespace ::testing;

TEST(OutputTests, set) 
{
    OutputMock<int> test_output;
    Pipeline::OutputRef outputRef{test_output};

    EXPECT_CALL(test_output, type_id())
        .WillRepeatedly(testing::Return(Utils::type_id<int>()));
    EXPECT_CALL(test_output, set(100))
        .Times(1);
    EXPECT_CALL(test_output, set(200))
        .Times(1);

    int temp_value = 100;
    test_output.set(temp_value);

    temp_value = 200;
    test_output.set(std::move(temp_value));

}

TEST(OutputTests, bind_output_ref_to_input_ref) 
{
    OutputMock<int> test_output;
    Pipeline::OutputRef outputRef{test_output};

    EXPECT_CALL(test_output, type_id())
        .WillRepeatedly(testing::Return(Utils::type_id<int>()));
    // EXPECT_CALL(test_output, set(_)).Times(2);

    IO::InputMock<int> test_input;
    Pipeline::InputRef inputRef{test_input};

    EXPECT_CALL(test_input, compatible(Utils::type_id<int>()))
        .WillOnce(Return(true));
    EXPECT_CALL(test_input, notify(42))
        .Times(1);

    {
        auto deferred = bind(outputRef, inputRef);

        // test_output.set(45);
    }

    

    // test_output.set(42);
}


// TEST(OutputTests, wrong_type) 
// {
//     int stored_value = 5;

//     TestOutput<int> test_output{[&](int const& value) {
//         stored_value = value;
//     }, 
//     [&](int&& value) {
//         stored_value = std::move(value);
//     }};

//     Pipeline::OutputRef outputRef{test_output};

//     double wrong_type_value = 3.14;
//     EXPECT_THROW(outputRef.set(wrong_type_value), std::runtime_error);

//     double another_wrong_type_value = 2.71;
//     EXPECT_THROW(outputRef.set(std::move(another_wrong_type_value)), std::runtime_error);
// }