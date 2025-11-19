#include <gtest/gtest.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"
#include "ducta/ValueIO/TValueInput.hpp"
#include "ducta/ValueIO/TValueOutput.hpp"
#include "ducta/ValueIO/ValueIOBuilder.hpp"


using namespace ducta;

class Multiplier
{
public:
    struct Inputs
    {
        template <typename IOBuilder>
        Inputs(IOBuilder& builder)
        : left{builder.template make_input<int>()}
        , right{builder.template make_input<int>()}
        {}

        IO::TInput<int> left;
        IO::TInput<int> right;
    } inputs;

    struct Outputs
    {
        template <typename IOBuilder>
        Outputs(IOBuilder& builder)
        : result{builder.template make_output<int>()}
        {}

        IO::TOutput<int> result;
    } outputs;

public:
    template <typename IOBuilder>
    Multiplier(IOBuilder& builder)
    : inputs{builder}
    , outputs{builder}
    {}

    std::map<std::string, IO::InputRef> get_inputs()
    {
        return {{"left", inputs.left.get_ref()}, {"right", inputs.right.get_ref()}};
    }

    std::map<std::string, IO::OutputRef> get_outputs()
    {
        return {{"result", outputs.result.get_ref()}};
    }

    void process()
    {
        outputs.result = (inputs.left.value() * inputs.right.value());
    }
};

TEST(ValueIOTest, input_not_ready) 
{
    IO::ValueIOBuilder io_builder;

    IO::TOutput<int> left{io_builder.template make_output<int>()};
    IO::TOutput<int> right{io_builder.template make_output<int>()};
    IO::TInput<int> result{io_builder.template make_input<int>()};

    Multiplier mul{io_builder};

    std::vector<Utils::Deferred> connections;
    connections.emplace_back(bind(left.get_ref(), mul.get_inputs().at("left")));
    connections.emplace_back(bind(right.get_ref(), mul.get_inputs().at("right")));
    connections.emplace_back(bind(mul.get_outputs().at("result"), result.get_ref()));

    left = 10;
    right = 5;

    mul.process();

    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 50);
    
}