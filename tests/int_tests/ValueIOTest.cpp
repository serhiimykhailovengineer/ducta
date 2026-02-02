#include <gtest/gtest.h>

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"
#include "ducta/IO/Connection.hpp"

#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Core/Types/Vector.hpp"

using namespace ducta;

class Multiplier
{
public:
    struct Inputs
    {
        IO::TInput<int> left;
        IO::TInput<int> right;
    } inputs;

    struct Outputs
    {
        IO::TOutput<int> result;
    } outputs;

public:
    Map<StringView, IO::InputRef, 25> get_inputs()
    {
        Map<StringView, IO::InputRef, 25> inputs_ref;
        
        inputs_ref.insert(std::make_pair<StringView, IO::InputRef>("left", IO::InputRef{inputs.left}));
        inputs_ref.insert(std::make_pair<StringView, IO::InputRef>("right", IO::InputRef{inputs.right}));
        return inputs_ref;
    }

    Map<StringView, IO::OutputRef, 25> get_outputs()
    {
        Map<StringView, IO::OutputRef, 25> outputs_ref;
        
        outputs_ref.insert(std::make_pair<StringView, IO::OutputRef>("result", IO::OutputRef{outputs.result}));
        return outputs_ref;
    }

    void process()
    {
        outputs.result = (inputs.left.value() * inputs.right.value());
    }
};

TEST(ValueIOTest, multiplier_process) 
{
    IO::TOutput<int> left;
    IO::TOutput<int> right;
    IO::TInput<int> result;

    Multiplier mul;

    Vector<IO::Connection, 3> connections;
    {
        auto mul_inputs = mul.get_inputs();
        auto mul_left_it = mul_inputs.find("left");
        ASSERT_TRUE(mul_left_it != mul_inputs.end());

        ASSERT_NO_THROW(connections.emplace_back(bind(IO::OutputRef{left}, mul_left_it->second)));

        auto mul_right_it = mul_inputs.find("right");
        ASSERT_TRUE(mul_right_it != mul_inputs.end());
        ASSERT_NO_THROW(connections.emplace_back(bind(IO::OutputRef{right}, mul_right_it->second)));
    }
    {
        auto mul_outputs = mul.get_outputs();
        auto mul_result_it = mul_outputs.find("result");
        ASSERT_TRUE(mul_result_it != mul_outputs.end());

        ASSERT_NO_THROW(connections.emplace_back(bind(mul_result_it->second, IO::InputRef{result})));
    }

    left = 10;
    right = 5;

    mul.process();

    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 50);
}