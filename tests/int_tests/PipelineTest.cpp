#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ducta/Pipeline/Node.hpp"
#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/IO/OutputRef.hpp"

#include "Nodes/Adder.hpp"
#include "Nodes/Subtractor.hpp"
#include "Nodes/Divider.hpp"
#include "Nodes/Multiplier.hpp"

class TestClock 
{
public:
    TestClock(std::uint64_t epoch = 0)
    : _epoch(epoch), _now(epoch)
    {}

    std::uint64_t epoch()
    {
        return _epoch;
    }

    std::uint64_t now()
    {
        return _now;
    }

    void set_now(std::uint64_t value)
    {
        _now = value;
    }

private:
    std::uint64_t _epoch;
    std::uint64_t _now;
};



using namespace ::ducta;

TEST(PipelineTest, base_test)
{
    TestClock clock_mock;

    // Create nodes map
    std::map<std::string, Pipeline::Node> nodes{};

    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("add"), 
                  std::forward_as_tuple(std::in_place_type_t<ducta::Test::Adder<int>>{}));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("sub"), 
                  std::forward_as_tuple(std::in_place_type_t<ducta::Test::Subtractor<int>>{}));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("div"), 
                  std::forward_as_tuple(std::in_place_type_t<ducta::Test::Divider<int>>{}));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("mul"), 
                  std::forward_as_tuple(std::in_place_type_t<ducta::Test::Multiplier<int>>{}));

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    pipeline.configure(nodes,
        {"add", "mul", "div", "sub"});

    // Calculate (((2 + 2) * 3) / 2) - 10 = -4

    IO::TOutput<int> first;
    IO::TOutput<int> second;
    IO::TOutput<int> const_3;
    IO::TOutput<int> const_2;
    IO::TOutput<int> const_10;
    IO::TInput<int> result;
    
    std::vector<Utils::Deferred> cons{};

    cons.emplace_back(IO::bind(IO::OutputRef{first}, nodes.at("add").get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{second}, nodes.at("add").get_inputs().at("right")));
    cons.emplace_back(IO::bind(nodes.at("add").get_outputs().at("result"), nodes.at("mul").get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_3}, nodes.at("mul").get_inputs().at("right")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.at("mul").get_outputs().at("result")}, nodes.at("div").get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_2}, nodes.at("div").get_inputs().at("denominator")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.at("div").get_outputs().at("result")}, nodes.at("sub").get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_10}, nodes.at("sub").get_inputs().at("right")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.at("sub").get_outputs().at("result")}, IO::InputRef{result}));

    first.set(2);
    second.set(2);
    const_3.set(3);
    const_2.set(2);
    const_10.set(10);

    pipeline.init();
    pipeline.iterate();
    pipeline.release();

    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), -4);
}