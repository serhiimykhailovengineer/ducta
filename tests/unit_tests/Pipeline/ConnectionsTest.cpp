#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/Pipeline/ClockMock.hpp"
#include "ducta/Pipeline/Pipeline.hpp"

using namespace ducta;
using namespace ducta::Configuration;

namespace ConnectionsTest {

struct Source
{
    IO::TOutput<int> out;
    IO::TOutput<double> real;
    void init()
    {
    }
    void release()
    {
    }
};

struct Sink
{
    IO::TInput<int> in1;
    IO::TInput<int> in2;
    void init()
    {
    }
    void release()
    {
    }
};

} // namespace ConnectionsTest

DEFINE_NODE_OUTPUTS(ConnectionsTest::Source, ("out", out), ("real", real));
DEFINE_NODE_INPUTS(ConnectionsTest::Sink, ("in1", in1), ("in2", in2));

namespace {

template <class StringType>
std::string str(StringType const& s)
{
    return std::string{s.data(), s.size()};
}

std::string link(Pipeline::PortLink const& l)
{
    return str(l.output.node) + "." + str(l.output.name) + " -> " + str(l.input.node) + "." +
           str(l.input.name);
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

/* Decodes the "connections" member of { "connections": <json> } */
class Decoded
{
public:
    explicit Decoded(char const* json)
    {
        std::string const text = std::string{"{ \"connections\": "} + json + " }";
        EXPECT_TRUE(loadJson(StringView{text.data(), text.size()}, tree, diagnostics)) << json;
        ok = decode(tree.root().child("connections"), links, diagnostics);
    }

    PropTree tree;
    Diagnostics diagnostics;
    Pipeline::PortLinks links;
    bool ok{false};
};

} // namespace

// ========== Decoding ==========

TEST(ConnectionsTest, decodes_outputs_to_inputs_in_order)
{
    Decoded decoded{R"([{ "a.out": ["b.in1", "c.in1"] }, { "c.out": ["b.in2"] }])"};

    ASSERT_TRUE(decoded.ok) << describe(decoded.diagnostics);
    ASSERT_EQ(decoded.links.size(), 3u);
    EXPECT_EQ(link(decoded.links[0]), "a.out -> b.in1");
    EXPECT_EQ(link(decoded.links[1]), "a.out -> c.in1");
    EXPECT_EQ(link(decoded.links[2]), "c.out -> b.in2");
}

TEST(ConnectionsTest, several_outputs_in_one_object_and_single_input_string)
{
    Decoded decoded{R"([{ "a.out": "b.in", "a.err": ["log.in"] }])"};

    ASSERT_TRUE(decoded.ok) << describe(decoded.diagnostics);
    ASSERT_EQ(decoded.links.size(), 2u);
    EXPECT_EQ(link(decoded.links[0]), "a.out -> b.in");
    EXPECT_EQ(link(decoded.links[1]), "a.err -> log.in");
}

TEST(ConnectionsTest, node_names_may_contain_dots)
{
    Decoded decoded{R"([{ "audio.left.out": ["mix.main.in"] }])"};

    ASSERT_TRUE(decoded.ok) << describe(decoded.diagnostics);
    ASSERT_EQ(decoded.links.size(), 1u);
    EXPECT_EQ(str(decoded.links[0].output.node), "audio.left");
    EXPECT_EQ(str(decoded.links[0].output.name), "out");
    EXPECT_EQ(str(decoded.links[0].input.name), "in");
    EXPECT_EQ(str(decoded.links[0].input.node), "mix.main");
}

TEST(ConnectionsTest, invalid_ports_are_reported_at_their_location)
{
    Decoded decoded{"[\n  { \"aout\": [\"b.in\"] },\n  { \"a.out\": [\"b.\", \".in\", 5] }\n]"};

    EXPECT_FALSE(decoded.ok);
    ASSERT_EQ(decoded.diagnostics.size(), 4u) << describe(decoded.diagnostics);
    EXPECT_EQ(
        str(decoded.diagnostics[0].message), "invalid output 'aout' (expected \"<node>.<output>\")");
    EXPECT_EQ(decoded.diagnostics[0].location.line(), 2u);
    EXPECT_EQ(
        str(decoded.diagnostics[1].message), "invalid input 'b.' (expected \"<node>.<input>\")");
    EXPECT_EQ(decoded.diagnostics[1].location.line(), 3u);
    EXPECT_EQ(
        str(decoded.diagnostics[2].message), "invalid input '.in' (expected \"<node>.<input>\")");
    EXPECT_EQ(str(decoded.diagnostics[3].message), "expected string, got integer");
}

TEST(ConnectionsTest, malformed_elements)
{
    EXPECT_EQ(
        str(Decoded{"[{}]"}.diagnostics[0].message),
        "expected { \"<node>.<output>\": [\"<node>.<input>\", ...] }");
    EXPECT_EQ(
        str(Decoded{R"([{ "a.out": [] }])"}.diagnostics[0].message),
        "output 'a.out' is connected to no input");
    EXPECT_EQ(str(Decoded{R"(["a.out"])"}.diagnostics[0].message), "expected object, got string");
    EXPECT_EQ(
        str(Decoded{R"({ "a.out": ["b.in"] })"}.diagnostics[0].message),
        "expected array, got object");
}

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
TEST(ConnectionsTest, too_many_connections)
{
    std::string json = R"([{ "a.out": [)";
    for (int i = 0; i <= DUCTA_MAX_CONNECTIONS; ++i)
    {
        json += (i ? ", \"b.in" : "\"b.in") + std::to_string(i) + "\"";
    }
    json += "] }]";
    Decoded decoded{json.c_str()};

    EXPECT_FALSE(decoded.ok);
    EXPECT_EQ(decoded.links.size(), static_cast<size_t>(DUCTA_MAX_CONNECTIONS));
    EXPECT_EQ(
        str(decoded.diagnostics[0].message),
        "too many connections (max " + std::to_string(DUCTA_MAX_CONNECTIONS) + ")");
}
#endif

// ========== Wiring by Pipeline::configure ==========

class PipelineConnectionsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        nodes.add_node("src", source);
        nodes.add_node("dst", sink);
        config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("src", "dst");
    }

    void connect(char const* output, char const* input)
    {
        Pipeline::PortLink link;
        ASSERT_TRUE(Pipeline::Private::parse_port(output, link.output)) << output;
        ASSERT_TRUE(Pipeline::Private::parse_port(input, link.input)) << input;
        config.connections.push_back(link);
    }

    ::testing::NiceMock<Pipeline::ClockMock> clock;
    ConnectionsTest::Source source;
    ConnectionsTest::Sink sink;
    Pipeline::Nodes nodes;
    Pipeline::Pipeline::Config config;
    Diagnostics diagnostics;
    Pipeline::Pipeline pipeline{clock};
};

TEST_F(PipelineConnectionsTest, connects_one_output_to_several_inputs)
{
    connect("src.out", "dst.in1");
    connect("src.out", "dst.in2");

    ASSERT_TRUE(pipeline.configure(nodes, config, diagnostics)) << describe(diagnostics);
    source.out = 7;

    ASSERT_TRUE(sink.in1.ready());
    ASSERT_TRUE(sink.in2.ready());
    EXPECT_EQ(sink.in1.value(), 7);
    EXPECT_EQ(sink.in2.value(), 7);
}

TEST_F(PipelineConnectionsTest, clear_and_reconfigure_disconnect)
{
    connect("src.out", "dst.in1");
    ASSERT_TRUE(pipeline.configure(nodes, config, diagnostics));

    pipeline.clear();
    source.out = 1;
    EXPECT_FALSE(sink.in1.ready());

    config.connections.clear();
    connect("src.out", "dst.in2");
    ASSERT_TRUE(pipeline.configure(nodes, config, diagnostics));
    source.out = 2;
    EXPECT_FALSE(sink.in1.ready());
    ASSERT_TRUE(sink.in2.ready());
    EXPECT_EQ(sink.in2.value(), 2);
}

TEST_F(PipelineConnectionsTest, all_problems_are_reported_and_nothing_is_connected)
{
    connect("src.out", "dst.in1"); // fine, but dropped because of the errors below
    connect("nope.out", "dst.in2"); // unknown node
    connect("src.missing", "dst.in2"); // unknown output
    connect("src.out", "dst.missing"); // unknown input
    connect("src.real", "dst.in2"); // double -> int
    connect("src.out", "dst.in1"); // in1 connected twice

    EXPECT_FALSE(pipeline.configure(nodes, config, diagnostics));

    ASSERT_EQ(diagnostics.size(), 5u) << describe(diagnostics);
    EXPECT_EQ(str(diagnostics[0].message), "unknown node 'nope' in connections");
    EXPECT_EQ(str(diagnostics[1].message), "node 'src' has no output 'missing'");
    EXPECT_EQ(str(diagnostics[2].message), "node 'dst' has no input 'missing'");
    EXPECT_EQ(
        str(diagnostics[3].message), "cannot connect 'src.real' to 'dst.in2': incompatible types");
    EXPECT_EQ(str(diagnostics[4].message), "input 'dst.in1' is connected more than once");

    source.out = 3;
    EXPECT_FALSE(sink.in1.ready());
}

TEST_F(PipelineConnectionsTest, configure_applies_the_config_rules)
{
    Pipeline::Pipeline::Config empty;
    EXPECT_FALSE(pipeline.configure(nodes, empty, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not be empty");

    diagnostics.clear();
    config.order.push_back(Pipeline::NodeName{"src"});
    EXPECT_FALSE(pipeline.configure(nodes, config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not contain a node twice");

    diagnostics.clear();
    config.order.pop_back();
    config.trace_flush_threshold = DUCTA_MAX_TRACED_FRAMES + 1;
    EXPECT_FALSE(pipeline.configure(nodes, config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not exceed DUCTA_MAX_TRACED_FRAMES");
}

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
TEST(OutputCapacityTest, full_output_refuses_further_inputs)
{
    IO::TOutput<int> output;
    std::vector<IO::TInput<int>> inputs(DUCTA_MAX_OUTPUT_CONNECTIONS + 1);
    std::vector<IO::Connection> connections;
    for (size_t i = 0; i < DUCTA_MAX_OUTPUT_CONNECTIONS; ++i)
    {
        connections.push_back(IO::bind(IO::OutputRef{output}, IO::InputRef{inputs[i]}));
        ASSERT_TRUE(static_cast<bool>(connections.back())) << i;
    }
    EXPECT_TRUE(output.full());
    EXPECT_TRUE(IO::OutputRef{output}.full());

    // One more is refused instead of overflowing the fixed vector
    IO::Connection extra = IO::bind(IO::OutputRef{output}, IO::InputRef{inputs.back()});
    EXPECT_FALSE(static_cast<bool>(extra));
    output = 1;
    EXPECT_FALSE(inputs.back().ready());

    // Unbinding one makes room again
    connections.pop_back();
    EXPECT_FALSE(output.full());
}

TEST_F(PipelineConnectionsTest, inputs_bound_outside_the_configuration_count_for_the_capacity)
{
    // The output already feeds the maximum number of inputs, bound by hand
    std::vector<IO::TInput<int>> probes(DUCTA_MAX_OUTPUT_CONNECTIONS);
    std::vector<IO::Connection> manual;
    for (auto& probe : probes)
    {
        manual.push_back(IO::bind(IO::OutputRef{source.out}, IO::InputRef{probe}));
    }
    connect("src.out", "dst.in1");

    EXPECT_FALSE(pipeline.configure(nodes, config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u) << describe(diagnostics);
    EXPECT_EQ(
        str(diagnostics[0].message),
        "output 'src.out' is connected to more than " +
            std::to_string(DUCTA_MAX_OUTPUT_CONNECTIONS) + " inputs");
}
#endif

TEST_F(PipelineConnectionsTest, unknown_node_in_order)
{
    config.order.push_back(Pipeline::NodeName{"ghost"});

    EXPECT_FALSE(pipeline.configure(nodes, config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "unknown node 'ghost' in pipeline order");
}

TEST(ConnectionTest, reassigning_a_connection_unbinds_the_previous_input)
{
    IO::TOutput<int> output;
    IO::TInput<int> first;
    IO::TInput<int> second;

    IO::Connection connection = IO::bind(IO::OutputRef{output}, IO::InputRef{first});
    connection = IO::bind(IO::OutputRef{output}, IO::InputRef{second});
    output = 5;

    EXPECT_FALSE(first.ready());
    ASSERT_TRUE(second.ready());
    EXPECT_EQ(second.value(), 5);

    // value() consumed 5; once disconnected, 6 must not arrive
    connection = IO::Connection{};
    output = 6;
    EXPECT_FALSE(second.ready());
}
