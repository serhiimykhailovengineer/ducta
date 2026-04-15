#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ducta/Pipeline/NodeRef.hpp"
#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/IO/OutputRef.hpp"

#include "Nodes/Adder.hpp"
#include "Nodes/Subtractor.hpp"
#include "Nodes/Divider.hpp"
#include "Nodes/Multiplier.hpp"

class TestClock 
{
public:
    using Timestamp = ducta::Chrono::TimestampUS;

public:
    TestClock(Timestamp epoch = Timestamp{0})
    : _epoch(epoch), _now(epoch)
    {}

    Timestamp epoch()
    {
        return _epoch;
    }

    Timestamp now()
    {
        return _now;
    }

    void sleepFor(Timestamp duration)
    {
        _now += duration;
    }

    void set_now(Timestamp value)
    {
        _now = value;
    }

private:
    Timestamp _epoch;
    Timestamp _now;
};



using namespace ::ducta;

TEST(PipelineTest, base_test)
{
    TestClock clock_mock;

    ducta::Test::Adder<int> adder{};
    ducta::Test::Subtractor<int> subtractor{};
    ducta::Test::Divider<int> divider{};
    ducta::Test::Multiplier<int> multiplier{};


    // Create nodes map
    Pipeline::Nodes nodes;

    nodes.add_node("add", adder);
    nodes.add_node("sub", subtractor);
    nodes.add_node("div", divider);
    nodes.add_node("mul", multiplier);
    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    Pipeline::Pipeline::Config config{};
    config.order = makeVector<StringView>("add", "mul", "div", "sub");

    pipeline.configure(nodes, config);

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

    ducta::Test::Adder<int> adder{};
    ducta::Test::Subtractor<int> subtractor{};

    // Create nodes map
    Pipeline::Nodes nodes;


    nodes.add_node("add", adder);
    nodes.add_node("sub", subtractor);

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    Pipeline::Pipeline::Config config{};
    config.order = makeVector<StringView>("add", "sub");

    pipeline.configure(nodes, config);

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