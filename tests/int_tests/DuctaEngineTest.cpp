// Integration tests: configuration text -> DuctaEngine -> connected nodes -> run -> results

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/DuctaEngine.hpp"
#include "ducta/IO/Connection.hpp"
#include "ducta/IO/OutputRef.hpp"

#include "Nodes/EngineNodes.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <cstdio>
#include <fstream>
#endif

using namespace ::ducta;
using namespace ::ducta::Configuration;

namespace {

class EngineClock
{
public:
    using Timestamp = Chrono::TimestampUS;

    Timestamp epoch()
    {
        return Timestamp{0};
    }

    Timestamp now()
    {
        return m_now;
    }

    void sleepFor(Timestamp duration)
    {
        m_now += duration;
        m_slept += duration;
    }

    Timestamp slept() const
    {
        return m_slept;
    }

private:
    Timestamp m_now{0};
    Timestamp m_slept{0};
};

/* Requests stop after the given number of iterations */
class StopAfter
{
public:
    explicit StopAfter(int iterations)
        : m_left{iterations}
    {
    }

    bool stopRequested() const
    {
        return m_left-- <= 0;
    }

private:
    mutable int m_left;
};

/* Never requests stop: the pipeline runs until a node stops it */
struct NeverStop
{
    bool stopRequested() const
    {
        return false;
    }
};

template <class StringType>
std::string str(StringType const& s)
{
    return std::string{s.data(), s.size()};
}

std::string describe(Diagnostics const& diagnostics)
{
    std::ostringstream out;
    for (Diagnostic const& diagnostic : diagnostics)
    {
        out << diagnostic << '\n';
    }
    return out.str();
}

/* Wires node outputs to node inputs (and to the test's own inputs) after configure() */
class Wiring
{
public:
    explicit Wiring(Pipeline::Nodes& nodes)
        : m_nodes{nodes}
    {
    }

    void connect(char const* from, char const* output, char const* to, char const* input)
    {
        auto source = m_nodes.get_node(from);
        auto target = m_nodes.get_node(to);
        ASSERT_TRUE(source.has_value()) << from;
        ASSERT_TRUE(target.has_value()) << to;
        m_connections.push_back(
            IO::bind(source->node.get_outputs().at(output), target->node.get_inputs().at(input)));
    }

    void connect(char const* from, char const* output, IO::TInput<int>& probe)
    {
        auto source = m_nodes.get_node(from);
        ASSERT_TRUE(source.has_value()) << from;
        m_connections.push_back(IO::bind(source->node.get_outputs().at(output), IO::InputRef{probe}));
    }

    /* Connections must be dropped before the nodes are destroyed (reconfigure / engine) */
    void clear()
    {
        m_connections.clear();
    }

private:
    Pipeline::Nodes& m_nodes;
    std::vector<IO::Connection> m_connections;
};

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
/* With the default capacities a node with one output takes ~840 bytes on the embedded backend
 * (TOutput keeps DUCTA_MAX_OUTPUT_CONNECTIONS inputs inline), so the tests size their storage
 * explicitly instead of relying on DUCTA_NODE_STORAGE_BYTES */
using TestStorage = Pipeline::StaticNodeStorage<16 * 1024, 16>;
#else
using TestStorage = Pipeline::HeapNodeStorage;
#endif

} // namespace

class DuctaEngineIntegration : public ::testing::Test
{
protected:
    /* The tree is a member: diagnostics point to its source name, so it must outlive them */
    bool configure(char const* json)
    {
        tree.clear();
        EXPECT_TRUE(loadJson(json, tree, diagnostics)) << describe(diagnostics);
        return engine.configure(tree, diagnostics);
    }

    EngineClock clock;
    PropTree tree;
    Diagnostics diagnostics;
    // Destroyed in reverse order: the connections go first, then the probe, then the nodes
    DuctaEngine<TestStorage> engine{clock};
    IO::TInput<int> result;
    Wiring wiring{engine.nodes()};
};

TEST_F(DuctaEngineIntegration, arithmetic_graph)
{
    // (2 + 3) * 4 * 10 = 200
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "two":   { "type": "Constant", "config": { "value": 2 } },
            "three": { "type": "Constant", "config": { "value": 3 } },
            "four":  { "type": "Constant", "config": { "value": 4 } },
            "sum":   { "type": "Add" },
            "mul":   { "type": "Multiply" },
            "x10":   { "type": "Scale", "config": { "factor": 10 } }
        },
        "pipeline": { "order": ["two", "three", "four", "sum", "mul", "x10"] }
    })"))
        << describe(diagnostics);
    EXPECT_TRUE(diagnostics.empty()) << describe(diagnostics);

    wiring.connect("two", "out", "sum", "left");
    wiring.connect("three", "out", "sum", "right");
    wiring.connect("sum", "result", "mul", "left");
    wiring.connect("four", "out", "mul", "right");
    wiring.connect("mul", "result", "x10", "in");
    wiring.connect("x10", "out", result);

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 200);
}

TEST_F(DuctaEngineIntegration, instances_of_one_type_with_same_and_different_configs)
{
    // 5 * 2 * 2 * 3 = 60: "double_a" and "double_b" share a configuration, "triple" differs
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "five":     { "type": "Constant", "config": { "value": 5 } },
            "double_a": { "type": "Scale", "config": { "factor": 2 } },
            "double_b": { "type": "Scale", "config": { "factor": 2 } },
            "triple":   { "type": "Scale", "config": { "factor": 3 } }
        },
        "pipeline": { "order": ["five", "double_a", "double_b", "triple"] }
    })"))
        << describe(diagnostics);

    wiring.connect("five", "out", "double_a", "in");
    wiring.connect("double_a", "out", "double_b", "in");
    wiring.connect("double_b", "out", "triple", "in");
    wiring.connect("triple", "out", result);

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 60);
}

TEST_F(DuctaEngineIntegration, order_defines_data_flow_within_one_iteration)
{
    // The consumer runs before the producer: the value arrives one iteration later
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "seven": { "type": "Constant", "config": { "value": 7 } },
            "x2":    { "type": "Scale", "config": { "factor": 2 } }
        },
        "pipeline": { "order": ["x2", "seven"] }
    })"))
        << describe(diagnostics);

    wiring.connect("seven", "out", "x2", "in");
    wiring.connect("x2", "out", result);

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_FALSE(result.ready());

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 14);
}

TEST_F(DuctaEngineIntegration, node_stops_the_pipeline)
{
    ASSERT_TRUE(configure(R"({
        "nodes": { "countdown": { "type": "Countdown", "config": { "iterations": 5 } } },
        "pipeline": { "order": ["countdown"] }
    })"))
        << describe(diagnostics);

    wiring.connect("countdown", "count", result);

    EXPECT_EQ(engine.run(NeverStop{}), EXIT_SUCCESS);
    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 5);
}

TEST_F(DuctaEngineIntegration, events_between_configured_instances)
{
    // Two tickers of one type with different steps feed one accumulator through events,
    // which the engine wires automatically
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "ones": { "type": "Ticker", "config": { "step": 1 } },
            "tens": { "type": "Ticker", "config": { "step": 10 } },
            "sum":  { "type": "Accumulator" }
        },
        "pipeline": { "order": ["ones", "tens", "sum"] }
    })"))
        << describe(diagnostics);

    wiring.connect("sum", "total", result);

    EXPECT_EQ(engine.run(StopAfter{3}), EXIT_SUCCESS);
    ASSERT_TRUE(result.ready());
    EXPECT_EQ(result.value(), 33);
}

TEST_F(DuctaEngineIntegration, frame_duration_from_configuration)
{
    ASSERT_TRUE(configure(R"({
        "nodes": { "one": { "type": "Constant", "config": { "value": 1 } } },
        "pipeline": { "order": ["one"], "target_frame_duration": "2.5ms" }
    })"))
        << describe(diagnostics);

    EXPECT_EQ(engine.run(StopAfter{4}), EXIT_SUCCESS);
    EXPECT_EQ(clock.slept(), Chrono::TimestampUS{10000});
}

TEST_F(DuctaEngineIntegration, reconfigure_with_another_graph)
{
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "one": { "type": "Constant", "config": { "value": 1 } },
            "x5":  { "type": "Scale", "config": { "factor": 5 } }
        },
        "pipeline": { "order": ["one", "x5"] }
    })"))
        << describe(diagnostics);
    wiring.connect("one", "out", "x5", "in");
    wiring.connect("x5", "out", result);
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(result.value(), 5);

    // The old nodes are destroyed by configure(): drop their connections first
    wiring.clear();
    ASSERT_TRUE(configure(R"({
        "nodes": {
            "two":   { "type": "Constant", "config": { "value": 2 } },
            "three": { "type": "Constant", "config": { "value": 3 } },
            "sum":   { "type": "Add" }
        },
        "pipeline": { "order": ["two", "three", "sum"] }
    })"))
        << describe(diagnostics);
    EXPECT_FALSE(engine.nodes().get_node("x5").has_value());
    wiring.connect("two", "out", "sum", "left");
    wiring.connect("three", "out", "sum", "right");
    wiring.connect("sum", "result", result);

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(result.value(), 5);
}

TEST_F(DuctaEngineIntegration, all_configuration_problems_are_reported)
{
    EXPECT_FALSE(configure(R"({
        "nodes": {
            "a": { "type": "Countdown", "config": { "iterations": 0 } },
            "b": { "type": "Scale", "config": { "factor": 2, "factr": 3 } },
            "c": { "type": "Teleporter", "config": {} }
        },
        "pipeline": { "order": ["a", "b", "c"], "target_frame_duration": "5 parsecs" }
    })"));

    EXPECT_FALSE(engine.configured());
    EXPECT_EQ(engine.node_manager().size(), 0u);
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);

    // Decoding stops at the first section with errors; the duration error is reported with its line
    ASSERT_EQ(diagnostics.error_count(), 1u) << describe(diagnostics);
    EXPECT_NE(describe(diagnostics).find("7:"), std::string::npos) << describe(diagnostics);
    EXPECT_NE(describe(diagnostics).find("pipeline.target_frame_duration"), std::string::npos)
        << describe(diagnostics);

    // Fix the pipeline section: now every node problem is reported, the typo as a warning
    diagnostics.clear();
    EXPECT_FALSE(configure(R"({
        "nodes": {
            "a": { "type": "Countdown", "config": { "iterations": 0 } },
            "b": { "type": "Scale", "config": { "factor": 2, "factr": 3 } },
            "c": { "type": "Teleporter", "config": {} }
        },
        "pipeline": { "order": ["a", "b", "c"] }
    })"));
    std::string const report = describe(diagnostics);
    // Text sources have no file name: every line starts with "line:column:"
    EXPECT_EQ(report.rfind("3:67: ", 0), 0u) << report;
    EXPECT_EQ(diagnostics.error_count(), 2u) << report;
    EXPECT_EQ(diagnostics.warning_count(), 1u) << report;
    EXPECT_NE(report.find("3:67: nodes.a.config.iterations: must be positive"), std::string::npos)
        << report;
    EXPECT_NE(report.find("4:71: nodes.b.config.factr: warning: unknown field 'factr'"), std::string::npos)
        << report;
    EXPECT_NE(
        report.find("5:52: nodes.c.config: unknown node type 'Teleporter' of node 'c'"),
        std::string::npos)
        << report;
}

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
TEST_F(DuctaEngineIntegration, configuration_from_file)
{
    std::string const path = ::testing::TempDir() + "ducta_engine_test.json";
    {
        std::ofstream file{path};
        file << R"({
    "nodes": {
        "six":  { "type": "Constant", "config": { "value": 6 } },
        "x7":   { "type": "Scale", "config": { "factor": 7 } },
        "fast": { "type": "Scale", "config": { "factor": -1 } }
    },
    "pipeline": { "order": ["six", "x7"], "target_frame_duration": "1ms" }
})";
    }

    ASSERT_TRUE(loadJsonFile(path, tree, diagnostics)) << describe(diagnostics);
    ASSERT_TRUE(engine.configure(tree, diagnostics)) << describe(diagnostics);

    // "fast" is created but not run: reported with the file name and line
    ASSERT_EQ(diagnostics.size(), 1u);
    std::ostringstream warning;
    warning << diagnostics[0];
    EXPECT_EQ(
        warning.str(),
        path +
            ":5:46: nodes.fast.config: warning: node 'fast' is not in pipeline order and never "
            "runs");

    wiring.connect("six", "out", "x7", "in");
    wiring.connect("x7", "out", result);
    EXPECT_EQ(engine.run(StopAfter{2}), EXIT_SUCCESS);
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(clock.slept(), Chrono::TimestampUS{2000});

    std::remove(path.c_str());
}
#endif
