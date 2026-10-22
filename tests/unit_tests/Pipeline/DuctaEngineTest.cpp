#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/DuctaEngine.hpp"
#include "ducta/Utils/Log.hpp"
#include "ducta/Utils/Log/CallbackSink.hpp"

#include "TestNodes.hpp"

using namespace ducta;
using namespace ducta::Configuration;
using namespace ducta::Pipeline;

namespace {

class TestClock
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

PropTree loadTree(char const* text)
{
    PropTree tree;
    Diagnostics diagnostics;
    EXPECT_TRUE(loadJson(text, tree, diagnostics)) << text;
    return tree;
}

template <class StringType>
std::string str(StringType const& s)
{
    return std::string{s.data(), s.size()};
}

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

void add(DuctaEngineConfig& config, char const* name, char const* type, PropNode node = {})
{
    config.nodes.insert(
        ::ducta::pair<NodeName const, NodeSpec>(NodeName{name}, NodeSpec{NodeName{type}, node}));
}

} // namespace

template <class Storage>
class DuctaEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestNodes::run_log().clear();
        TestNodes::CountingNode::alive = 0;
        TestNodes::GainNode::last_gain = 0.0;
    }

    TestClock clock;
};

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using Storages = ::testing::Types<StaticNodeStorage<512, 8>>;
#else
using Storages = ::testing::Types<StaticNodeStorage<512, 8>, HeapNodeStorage>;
#endif
TYPED_TEST_SUITE(DuctaEngineTest, Storages);

TYPED_TEST(DuctaEngineTest, configure_from_prop_tree_and_run)
{
    PropTree tree = loadTree(R"({
        "nodes": { "First": { "type": "First" }, "Second": { "type": "Second" }, "Gain": { "type": "Gain", "config": { "gain": 3.0 } } },
        "pipeline": { "order": ["Second", "First", "Gain"], "target_frame_duration": "500us" }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(tree, diagnostics));
    EXPECT_TRUE(diagnostics.empty());
    EXPECT_DOUBLE_EQ(TestNodes::GainNode::last_gain, 3.0);

    EXPECT_TRUE(engine.configured());

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iBiABArArB");
    EXPECT_EQ(this->clock.slept(), Chrono::TimestampUS{500});
}

TYPED_TEST(DuctaEngineTest, several_instances_of_one_type_run_in_order)
{
    PropTree tree = loadTree(R"({
        "nodes": {
            "a1": { "type": "First" },
            "a2": { "type": "First" },
            "b":  { "type": "Second" }
        },
        "pipeline": { "order": ["a2", "b", "a1"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(tree, diagnostics));
    EXPECT_TRUE(diagnostics.empty());
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAiBiAABArArBrA");
}

TYPED_TEST(DuctaEngineTest, configure_from_config_and_run)
{
    DuctaEngineConfig config;
    add(config, "First", "First");
    add(config, "Second", "Second");
    config.pipeline.order.push_back(NodeName{"First"});
    config.pipeline.order.push_back(NodeName{"Second"});
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(config, diagnostics));
    EXPECT_TRUE(diagnostics.empty());

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAiBABrBrA");
}

TYPED_TEST(DuctaEngineTest, unknown_node_in_order_fails_and_keeps_no_node)
{
    PropTree tree = loadTree(
        "{ \"nodes\": { \"Counting\": { \"type\": \"Counting\" } },\n"
        "  \"pipeline\": { \"order\": [\"Counting\", \"Nope\"] } }");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));

    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    EXPECT_EQ(engine.node_manager().size(), 0u);
    EXPECT_FALSE(engine.configured());
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "unknown node 'Nope' in pipeline order");
}

TYPED_TEST(DuctaEngineTest, duplicate_node_in_order_fails)
{
    DuctaEngineConfig config;
    add(config, "First", "First");
    config.pipeline.order.push_back(NodeName{"First"});
    config.pipeline.order.push_back(NodeName{"First"});
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not contain a node twice");
}

TYPED_TEST(DuctaEngineTest, duplicate_node_in_order_is_reported_at_order)
{
    PropTree tree = loadTree(
        "{ \"nodes\": { \"First\": { \"type\": \"First\" } },\n"
        "  \"pipeline\": { \"order\": [\"First\", \"First\"] } }");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not contain a node twice");
    EXPECT_EQ(str(diagnostics[0].path), "pipeline.order");
    EXPECT_EQ(diagnostics[0].location.line(), 2u);
}

TYPED_TEST(DuctaEngineTest, node_missing_from_order_is_a_warning)
{
    PropTree tree = loadTree(R"({
        "nodes": { "First": { "type": "First" }, "Second": { "type": "Second" } },
        "pipeline": { "order": ["First"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].severity, Severity::Warning);
    EXPECT_EQ(str(diagnostics[0].message), "node 'Second' is not in pipeline order and never runs");
    EXPECT_TRUE(str(diagnostics[0].path).empty()); // no "config": no location

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAArA");
}

TYPED_TEST(DuctaEngineTest, sections_are_required)
{
    PropTree tree = loadTree(R"({ "nodes": { "First": { "type": "First" } } })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "missing required field 'pipeline'");
}

TYPED_TEST(DuctaEngineTest, pipeline_settings_rules_are_checked)
{
    PropTree tree = loadTree(R"({
        "nodes": { "First": { "type": "First" } },
        "pipeline": { "order": [], "target_frame_duration": "-1ms" }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 2u);
    EXPECT_EQ(str(diagnostics[0].path), "pipeline.order");
    EXPECT_EQ(str(diagnostics[0].message), "must not be empty");
    EXPECT_EQ(str(diagnostics[1].path), "pipeline.target_frame_duration");
    EXPECT_EQ(str(diagnostics[1].message), "must not be negative");

    // The same rules apply to a Config built in code
    DuctaEngineConfig config;
    add(config, "First", "First");
    diagnostics.clear();
    EXPECT_FALSE(engine.configure(config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not be empty");
}

TYPED_TEST(DuctaEngineTest, node_errors_fail)
{
    PropTree tree = loadTree(R"({
        "nodes": { "Counting": { "type": "Counting" }, "Gain": { "type": "Gain", "config": { "gain": 0 } } },
        "pipeline": { "order": ["Counting", "Gain"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].path), "nodes.Gain.config.gain");
}

TYPED_TEST(DuctaEngineTest, reconfigure_replaces_nodes_and_order)
{
    PropTree first = loadTree(R"({
        "nodes": { "Counting": { "type": "Counting" }, "First": { "type": "First" } },
        "pipeline": { "order": ["Counting", "First"] }
    })");
    PropTree second = loadTree(R"({
        "nodes": { "Second": { "type": "Second" } },
        "pipeline": { "order": ["Second"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(first, diagnostics));
    EXPECT_EQ(TestNodes::CountingNode::alive, 1);
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);

    ASSERT_TRUE(engine.configure(second, diagnostics));
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
    TestNodes::run_log().clear();
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iBBrB");

    engine.clear();
    EXPECT_EQ(engine.node_manager().size(), 0u);
    EXPECT_FALSE(engine.configured());
}

TYPED_TEST(DuctaEngineTest, destructor_destroys_nodes)
{
    DuctaEngineConfig config;
    add(config, "Counting", "Counting");
    config.pipeline.order.push_back(NodeName{"Counting"});
    Diagnostics diagnostics;
    {
        DuctaEngine<TypeParam> engine{this->clock};
        ASSERT_TRUE(engine.configure(config, diagnostics));
        EXPECT_EQ(TestNodes::CountingNode::alive, 1);
    }
    EXPECT_EQ(TestNodes::CountingNode::alive, 0);
}

#if DUCTA_ENABLE_TRACING
TYPED_TEST(DuctaEngineTest, trace_callback_receives_frames)
{
    PropTree tree = loadTree(R"({
        "nodes": { "First": { "type": "First" }, "Second": { "type": "Second" } },
        "pipeline": { "order": ["First", "Second"], "should_trace_execution": true, "trace_flush_threshold": 2 }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    size_t frames = 0;
    size_t names = 0;
    auto on_trace = [&](Span<StringView const> node_names, Span<FrameTraceInfo const> frames_info) {
        names = node_names.size();
        frames += frames_info.size();
    };
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    typename DuctaEngine<TypeParam>::TraceCallback callback = makeFunctionRef(on_trace);
#else
    typename DuctaEngine<TypeParam>::TraceCallback callback = on_trace;
#endif

    ASSERT_TRUE(engine.configure(tree, diagnostics, callback));
    EXPECT_EQ(engine.run(StopAfter{4}), EXIT_SUCCESS);
    EXPECT_EQ(names, 2u);
    EXPECT_EQ(frames, 4u);
}
#endif

TYPED_TEST(DuctaEngineTest, runs_until_stop_and_can_run_again)
{
    DuctaEngineConfig config;
    add(config, "First", "First");
    config.pipeline.order.push_back(NodeName{"First"});
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    ASSERT_TRUE(engine.configure(config, diagnostics));
    EXPECT_EQ(engine.run(StopAfter{3}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAAAArA");

    TestNodes::run_log().clear();
    EXPECT_EQ(engine.run(StopAfter{0}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iArA");
}

#if defined(__cpp_exceptions)
TYPED_TEST(DuctaEngineTest, engine_is_usable_after_a_node_throws_while_running)
{
    PropTree tree = loadTree(R"({
        "nodes": { "t": { "type": "Throwing", "config": { "in_init": true } } },
        "pipeline": { "order": ["t"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    ASSERT_TRUE(engine.configure(tree, diagnostics));

    // The exception stops at the node: the run fails, the engine is not stuck in "running"
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);

    // Not stuck in "running": it can be reconfigured and run again
    PropTree other =
        loadTree(R"({ "nodes": { "a": { "type": "First" } }, "pipeline": { "order": ["a"] } })");
    ASSERT_TRUE(engine.configure(other, diagnostics)) << diagnostics.size();
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAArA");
}
#endif

TYPED_TEST(DuctaEngineTest, tracing_without_callback_keeps_the_latest_frames)
{
    PropTree tree = loadTree(R"({
        "nodes": { "a": { "type": "First" } },
        "pipeline": { "order": ["a"], "should_trace_execution": true }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    ASSERT_TRUE(engine.configure(tree, diagnostics));

    // More frames than the trace buffer holds, and nobody to flush them
    EXPECT_EQ(engine.run(StopAfter{DUCTA_MAX_TRACED_FRAMES + 10}), EXIT_SUCCESS);
}

TYPED_TEST(DuctaEngineTest, trace_flush_threshold_must_fit_the_buffer)
{
    std::string const json = R"({ "nodes": { "a": { "type": "First" } }, "pipeline": { "order": ["a"], )"
                             R"("trace_flush_threshold": )" +
                             std::to_string(DUCTA_MAX_TRACED_FRAMES + 1) + " } }";
    PropTree tree = loadTree(json.c_str());
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configure(tree, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(str(diagnostics[0].message), "must not exceed DUCTA_MAX_TRACED_FRAMES");
}

#if defined(__cpp_exceptions)
TYPED_TEST(DuctaEngineTest, nodes_initialised_before_a_failing_init_are_released)
{
    PropTree tree = loadTree(R"({
        "nodes": {
            "a": { "type": "First" },
            "b": { "type": "Second" },
            "t": { "type": "Throwing", "config": { "in_init": true } }
        },
        "pipeline": { "order": ["a", "b", "t"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    ASSERT_TRUE(engine.configure(tree, diagnostics));

    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iAiBrBrA");
}
#endif

TYPED_TEST(DuctaEngineTest, default_trace_flush_threshold_is_valid)
{
    EXPECT_LE(
        ::ducta::Pipeline::PipelineConfig{}.trace_flush_threshold,
        static_cast<size_t>(DUCTA_MAX_TRACED_FRAMES));

    PropTree tree = loadTree(R"({
        "nodes": { "a": { "type": "First" } },
        "pipeline": { "order": ["a"], "should_trace_execution": true }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    EXPECT_TRUE(engine.configure(tree, diagnostics));
}

#if defined(__cpp_exceptions)
TYPED_TEST(DuctaEngineTest, node_throwing_from_release_is_logged_and_the_others_are_released)
{
    PropTree tree = loadTree(R"({
        "nodes": {
            "a": { "type": "First" },
            "t": { "type": "Throwing", "config": { "in_release": true } },
            "b": { "type": "Second" }
        },
        "pipeline": { "order": ["a", "t", "b"] }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    ASSERT_TRUE(engine.configure(tree, diagnostics));

    // release() of a node must not throw: if it does, the failure is logged with the node name
    // and "a" (released after it) is still released
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAiBABrBrA");

    // The next run starts from scratch: every node is initialised again
    TestNodes::run_log().clear();
    EXPECT_EQ(engine.run(StopAfter{0}), EXIT_SUCCESS);
    EXPECT_EQ(TestNodes::run_log(), "iAiBrBrA");
}

TYPED_TEST(DuctaEngineTest, traced_run_survives_a_non_standard_exception_of_iterate)
{
    PropTree tree = loadTree(R"({
        "nodes": {
            "a": { "type": "First" },
            "t": { "type": "Throwing", "config": { "in_iterate": true } }
        },
        "pipeline": { "order": ["a", "t"], "should_trace_execution": true }
    })");
    Diagnostics diagnostics;
    DuctaEngine<TypeParam> engine{this->clock};
    ASSERT_TRUE(engine.configure(tree, diagnostics));

    // The exception stops at the node: the run fails, the nodes are released
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iAArA");

    // The trace frame of the failed iteration was closed: the next frame can start
    TestNodes::run_log().clear();
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iAArA");
}

TEST(PipelineEngineTest, destructor_releases_the_nodes_after_an_exception_of_iterate)
{
    TestClock clock;
    TestNodes::ThrowingConfig config;
    config.in_iterate = true;
    TestNodes::ThrowingNode node{config};
    TestNodes::LoggingNode<'A'> first;
    ::ducta::Pipeline::Nodes nodes;
    nodes.add_node("a", first);
    nodes.add_node("t", node);
    ::ducta::Pipeline::Pipeline::Config pipeline_config;
    pipeline_config.order.push_back(NodeName{"a"});
    pipeline_config.order.push_back(NodeName{"t"});
    ::ducta::Pipeline::Pipeline pipeline{clock};
    ASSERT_TRUE(pipeline.configure(nodes, pipeline_config));

    TestNodes::run_log().clear();
    {
        StopAfter stop{2};
        ::ducta::Pipeline::PipelineEngine engine{pipeline, stop};
        EXPECT_EQ(TestNodes::run_log(), "iA");
        EXPECT_EQ(engine.run(), EXIT_FAILURE);
        EXPECT_EQ(TestNodes::run_log(), "iAA"); // not released yet: the engine is alive
    }
    EXPECT_EQ(TestNodes::run_log(), "iAArA");
}

TEST(PipelineEngineTest, node_throwing_from_init_fails_and_the_nodes_before_it_are_released)
{
    TestClock clock;
    TestNodes::ThrowingConfig config;
    config.in_init = true;
    TestNodes::ThrowingNode node{config};
    TestNodes::LoggingNode<'A'> first;
    TestNodes::LoggingNode<'B'> last;
    ::ducta::Pipeline::Nodes nodes;
    nodes.add_node("a", first);
    nodes.add_node("t", node);
    nodes.add_node("b", last);
    ::ducta::Pipeline::Pipeline::Config pipeline_config;
    pipeline_config.order.push_back(NodeName{"a"});
    pipeline_config.order.push_back(NodeName{"t"});
    pipeline_config.order.push_back(NodeName{"b"});
    ::ducta::Pipeline::Pipeline pipeline{clock};
    ASSERT_TRUE(pipeline.configure(nodes, pipeline_config));

    TestNodes::run_log().clear();
    {
        StopAfter stop{1};
        ::ducta::Pipeline::PipelineEngine engine{pipeline, stop}; // the exception stops at "t"
        EXPECT_EQ(engine.run(), EXIT_FAILURE);
    }
    EXPECT_EQ(TestNodes::run_log(), "iArA"); // "t" failed: neither released nor "b" initialised
}

#endif

TEST(DuctaEngineLifetimeTest, temporary_factory_is_rejected)
{
    using Engine = DuctaEngine<>;
    static_assert(std::is_constructible_v<Engine, TestClock&, ::ducta::Pipeline::NodeFactory&>);
    static_assert(std::is_constructible_v<Engine, TestClock&, ::ducta::Pipeline::NodeFactory const&>);
    static_assert(!std::is_constructible_v<Engine, TestClock&, ::ducta::Pipeline::NodeFactory>);
    static_assert(
        !std::is_constructible_v<Engine, ::ducta::Pipeline::Clock, ::ducta::Pipeline::NodeFactory>);

    static_assert(
        std::is_constructible_v<::ducta::Pipeline::NodeManager<>, ::ducta::Pipeline::NodeFactory&>);
    static_assert(
        !std::is_constructible_v<::ducta::Pipeline::NodeManager<>, ::ducta::Pipeline::NodeFactory>);
    SUCCEED();
}

namespace {

/* init() / iterate() reporting a failure as a value */
struct FailingInitNode
{
    ::ducta::Expected<void, ::ducta::Error> init()
    {
        return ::ducta::Unexpected<::ducta::Error>{::ducta::Error{"no device"}};
    }
    void release()
    {
        TestNodes::run_log() += "rF";
    }
};

struct RefusingInitNode
{
    bool init()
    {
        return false;
    }
    void release()
    {
        TestNodes::run_log() += "rR";
    }
};

struct FailingIterateNode
{
    void init()
    {
    }
    ::ducta::Expected<void, ::ducta::Error> iterate()
    {
        return ::ducta::Unexpected<::ducta::Error>{::ducta::Error{"sensor lost"}};
    }
    void release()
    {
        TestNodes::run_log() += "rI";
    }
};

/* Records the error lines of the logger */
struct ErrorLines
{
    std::string text;
    ::ducta::Log::CallbackSink sink{
        +[](void* context, ::ducta::StringView line, ::ducta::Log::Level level) {
            if (level == ::ducta::Log::Level::Error)
            {
                static_cast<ErrorLines*>(context)->text.append(line.data(), line.size()).append("\n");
            }
        },
        this};

    ErrorLines()
    {
        ::ducta::Log::logger().add_sink(sink);
    }
    ~ErrorLines()
    {
        ::ducta::Log::logger().remove_sink(sink);
    }
};

template <class Middle>
void run_three_nodes(Middle& middle, ::ducta::Pipeline::Pipeline& pipeline, int expected_run_result)
{
    TestNodes::LoggingNode<'A'> first;
    TestNodes::LoggingNode<'B'> last;
    ::ducta::Pipeline::Nodes nodes;
    nodes.add_node("a", first);
    nodes.add_node("m", middle);
    nodes.add_node("b", last);
    ::ducta::Pipeline::Pipeline::Config pipeline_config;
    pipeline_config.order.push_back(NodeName{"a"});
    pipeline_config.order.push_back(NodeName{"m"});
    pipeline_config.order.push_back(NodeName{"b"});
    ASSERT_TRUE(pipeline.configure(nodes, pipeline_config));

    TestNodes::run_log().clear();
    StopAfter stop{3};
    ::ducta::Pipeline::PipelineEngine engine{pipeline, stop};
    EXPECT_EQ(engine.run(), expected_run_result);
}

} // namespace

TEST(PipelineEngineTest, error_of_init_stops_and_releases_the_nodes_initialised_before)
{
    TestClock clock;
    ::ducta::Pipeline::Pipeline pipeline{clock};
    FailingInitNode node;
    ErrorLines errors;
    run_three_nodes(node, pipeline, EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iArA"); // "m" failed: not released, "b" not initialised
    EXPECT_NE(errors.text.find("init() of node 'm' failed: no device"), std::string::npos)
        << errors.text;
}

TEST(PipelineEngineTest, init_returning_false_stops_and_releases_the_nodes_initialised_before)
{
    TestClock clock;
    ::ducta::Pipeline::Pipeline pipeline{clock};
    RefusingInitNode node;
    ErrorLines errors;
    run_three_nodes(node, pipeline, EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iArA");
    EXPECT_NE(errors.text.find("init() of node 'm' returned false"), std::string::npos)
        << errors.text;
}

TEST(PipelineEngineTest, error_of_iterate_stops_the_run_and_is_logged)
{
    TestClock clock;
    ::ducta::Pipeline::Pipeline pipeline{clock};
    FailingIterateNode node;
    ErrorLines errors;
    run_three_nodes(node, pipeline, EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iAiBArBrIrA"); // "b" not iterated
    EXPECT_NE(errors.text.find("iterate() of node 'm' failed: sensor lost"), std::string::npos)
        << errors.text;
}

#if defined(__cpp_exceptions)
TEST(PipelineEngineTest, node_breaking_its_contract_is_named_in_the_log)
{
    TestNodes::ThrowingConfig config;
    config.in_iterate = true; // throws 42: not even a std::exception
    config.in_release = true;
    TestNodes::ThrowingNode node{config};
    TestClock clock;
    ::ducta::Pipeline::Pipeline pipeline{clock};
    ErrorLines errors;
    run_three_nodes(node, pipeline, EXIT_FAILURE);
    EXPECT_EQ(TestNodes::run_log(), "iAiBArBrA"); // every other node released
    EXPECT_NE(errors.text.find("iterate() of node 'm' failed: unknown exception"), std::string::npos)
        << errors.text;
    EXPECT_NE(
        errors.text.find("release() of node 'm' failed: exception: release failed"),
        std::string::npos)
        << errors.text;
}
#endif

TYPED_TEST(DuctaEngineTest, run_without_configuration_fails)
{
    DuctaEngine<TypeParam> engine{this->clock};

    EXPECT_FALSE(engine.configured());
    EXPECT_EQ(engine.run(StopAfter{1}), EXIT_FAILURE);
    EXPECT_TRUE(TestNodes::run_log().empty());
}
