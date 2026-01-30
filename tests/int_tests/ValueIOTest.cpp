#include <gtest/gtest.h>

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/OutputRef.hpp"

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
    std::map<std::string, IO::InputRef> get_inputs()
    {
        return {{"left", IO::InputRef{inputs.left}}, {"right", IO::InputRef{inputs.right}}};
    }

    std::map<std::string, IO::OutputRef> get_outputs()
    {
        return {{"result", IO::OutputRef{outputs.result}}};
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

    std::vector<Deferred> connections;
    connections.emplace_back(bind(IO::OutputRef{left}, mul.get_inputs().at("left")));
    connections.emplace_back(bind(IO::OutputRef{right}, mul.get_inputs().at("right")));
    connections.emplace_back(bind(mul.get_outputs().at("result"), IO::InputRef{result}));

    left = 10;
    right = 5;

    mul.process();

    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 50);
}