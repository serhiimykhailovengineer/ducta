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
    Pipeline::Nodes nodes;

    nodes.add_node("add", ducta::Test::Adder<int>{});
    nodes.add_node("sub", ducta::Test::Subtractor<int>{});
    nodes.add_node("div", ducta::Test::Divider<int>{});
    nodes.add_node("mul", ducta::Test::Multiplier<int>{});

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    auto order = makeVector<StringView>("add", "mul", "div", "sub");

    pipeline.configure(nodes, order);

    // Calculate (((2 + 2) * 3) / 2) - 10 = -4

    IO::TOutput<int> first;
    IO::TOutput<int> second;
    IO::TOutput<int> const_3;
    IO::TOutput<int> const_2;
    IO::TOutput<int> const_10;
    IO::TInput<int> result;
    
    std::vector<IO::Connection> cons{};

    cons.emplace_back(IO::bind(IO::OutputRef{first}, nodes.get_node("add")->node.get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{second}, nodes.get_node("add")->node.get_inputs().at("right")));
    cons.emplace_back(IO::bind(nodes.get_node("add")->node.get_outputs().at("result"), nodes.get_node("mul")->node.get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_3}, nodes.get_node("mul")->node.get_inputs().at("right")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.get_node("mul")->node.get_outputs().at("result")}, nodes.get_node("div")->node.get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_2}, nodes.get_node("div")->node.get_inputs().at("denominator")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.get_node("div")->node.get_outputs().at("result")}, nodes.get_node("sub")->node.get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{const_10}, nodes.get_node("sub")->node.get_inputs().at("right")));
    cons.emplace_back(IO::bind(IO::OutputRef{nodes.get_node("sub")->node.get_outputs().at("result")}, IO::InputRef{result}));

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

TEST(PipelineTest, pipeline_with_failed_iteration)
{
    TestClock clock_mock;

    // Create nodes map
    Pipeline::Nodes nodes;

    nodes.add_node("add", ducta::Test::Adder<int>{});
    nodes.add_node("sub", ducta::Test::Subtractor<int>{});

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    auto order = makeVector<StringView>("add", "sub");

    pipeline.configure(nodes, order);

    IO::TOutput<int> first;
    IO::TOutput<int> second;
    IO::TInput<int> result;
    
    std::vector<IO::Connection> cons{};

    cons.emplace_back(IO::bind(IO::OutputRef{first}, nodes.get_node("add")->node.get_inputs().at("left")));
    cons.emplace_back(IO::bind(IO::OutputRef{second}, nodes.get_node("add")->node.get_inputs().at("right")));

    pipeline.init();
    EXPECT_FALSE(pipeline.iterate());
    pipeline.release();

    ASSERT_FALSE(result.ready());
}