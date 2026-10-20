#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeRef.hpp"
#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/Pipeline/ClockMock.hpp"
#include "ducta/Pipeline/NodeMock.hpp"
#include "ducta/Pipeline/StopTokenMock.hpp"

using namespace ducta;

TEST(PipelineTest, base_test)
{
    Pipeline::ClockMock clock_mock;

    Pipeline::NonIterableNodeMock node_1_mock;
    Pipeline::IterableNodeMock<void> node_2_mock;
    Pipeline::IterableNodeMock<void> node_3_mock;
    Pipeline::IterableNodeMock<void> node_4_mock;

    // Create nodes map
    Pipeline::Nodes nodes;

    nodes.add_node("node_1", node_1_mock);
    nodes.add_node("node_2", node_2_mock);
    nodes.add_node("node_3", node_3_mock);
    nodes.add_node("node_4", node_4_mock);

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    Pipeline::Pipeline::Config config{};
    config.order =
        makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("node_1", "node_2", "node_3", "node_4");

    pipeline.configure(nodes, config);

    // Testing initialization
    {
        ::testing::InSequence seq;
        EXPECT_CALL(node_1_mock, init()).Times(1);
        EXPECT_CALL(node_2_mock, init()).Times(1);
        EXPECT_CALL(node_3_mock, init()).Times(1);
        EXPECT_CALL(node_4_mock, init()).Times(1);
    }

    pipeline.init();

    // Testing iteration
    {
        ::testing::InSequence seq;

        // Without tracing and target frame duration the clock is read once per frame
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(node_2_mock, iterate()).Times(1);
        EXPECT_CALL(node_3_mock, iterate()).Times(1);
        EXPECT_CALL(node_4_mock, iterate()).Times(1);
    }

    EXPECT_TRUE(pipeline.iterate());

    // Testing release
    {
        ::testing::InSequence seq;

        EXPECT_CALL(node_4_mock, release()).Times(1);
        EXPECT_CALL(node_3_mock, release()).Times(1);
        EXPECT_CALL(node_2_mock, release()).Times(1);
        EXPECT_CALL(node_1_mock, release()).Times(1);
    }

    pipeline.release();
}

TEST(PipelineTest, iteration_with_failed_node)
{
    Pipeline::ClockMock clock_mock;

    Pipeline::IterableNodeMock<Expected<void, Error>> first_node_mock;
    Pipeline::IterableNodeMock<Expected<void, Error>> second_node_mock;
    // Create nodes map
    Pipeline::Nodes nodes;

    nodes.add_node("first", first_node_mock);
    nodes.add_node("second", second_node_mock);

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    Pipeline::Pipeline::Config config{};
    config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("first", "second");

    ASSERT_TRUE(pipeline.configure(nodes, config));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    EXPECT_CALL(clock_mock, now()).WillRepeatedly(::testing::Return(Chrono::TimestampUS{0}));
    EXPECT_CALL(first_node_mock, iterate()).WillOnce(::testing::Return(Unexpected<Error>{Error{}}));
    EXPECT_CALL(second_node_mock, iterate()).Times(0);
    EXPECT_CALL(second_node_mock, release()).Times(1);
    EXPECT_CALL(first_node_mock, release()).Times(1);

    pipeline.init();
    EXPECT_FALSE(pipeline.iterate());
    pipeline.release();
}

TEST(PipelineTest, pipeline_execution_stop_token)
{
    Pipeline::StopTokenMock stop_token_mock;

    EXPECT_CALL(stop_token_mock, stopRequested())
        .WillOnce(::testing::Return(false))
        .WillOnce(::testing::Return(true));

    Pipeline::ClockMock clock_mock;

    Pipeline::IterableNodeMock<Expected<void, Error>> first_node_mock;
    Pipeline::IterableNodeMock<Expected<void, Error>> second_node_mock;
    // Create nodes map
    Pipeline::Nodes nodes;

    nodes.add_node("first", first_node_mock);
    nodes.add_node("second", second_node_mock);

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    Pipeline::Pipeline::Config config{};
    config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("first", "second");

    ASSERT_TRUE(pipeline.configure(nodes, config));

    EXPECT_CALL(clock_mock, now()).WillRepeatedly(::testing::Return(Chrono::TimestampUS{0}));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    EXPECT_CALL(first_node_mock, iterate()).Times(1);
    EXPECT_CALL(second_node_mock, iterate()).Times(1);
    EXPECT_CALL(second_node_mock, release()).Times(1);
    EXPECT_CALL(first_node_mock, release()).Times(1);

    Pipeline::PipelineEngine engine{pipeline, stop_token_mock};
    EXPECT_EQ(engine.run(), EXIT_SUCCESS);
}
TEST(PipelineTest, target_frame_duration_reads_the_clock_again_and_sleeps_the_rest)
{
    Pipeline::ClockMock clock_mock;
    Pipeline::IterableNodeMock<void> first_node_mock;
    Pipeline::IterableNodeMock<void> second_node_mock;
    Pipeline::Nodes nodes;
    nodes.add_node("first", first_node_mock);
    nodes.add_node("second", second_node_mock);

    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};
    Pipeline::Pipeline::Config config{};
    config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("first", "second");
    config.target_frame_duration = Chrono::TimestampUS{1000};
    ASSERT_TRUE(pipeline.configure(nodes, config));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    pipeline.init();
    {
        ::testing::InSequence seq;
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{100}));
        EXPECT_CALL(first_node_mock, iterate()).Times(1);
        EXPECT_CALL(second_node_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{400}));
        EXPECT_CALL(clock_mock, sleepFor(Chrono::TimestampUS{700})).Times(1);
    }
    EXPECT_TRUE(pipeline.iterate());

    EXPECT_CALL(second_node_mock, release()).Times(1);
    EXPECT_CALL(first_node_mock, release()).Times(1);
    pipeline.release();
}

#if DUCTA_ENABLE_TRACING
TEST(PipelineTest, tracing_reads_the_clock_once_per_node)
{
    Pipeline::ClockMock clock_mock;
    Pipeline::IterableNodeMock<void> first_node_mock;
    Pipeline::IterableNodeMock<void> second_node_mock;
    Pipeline::Nodes nodes;
    nodes.add_node("first", first_node_mock);
    nodes.add_node("second", second_node_mock);

    std::vector<Chrono::TimestampUS> durations;
    Chrono::TimestampUS frame_start{-1};
    Chrono::TimestampUS frame_end{-1};
    auto on_trace = [&](Span<StringView const>, Span<Pipeline::FrameTraceInfo const> frames) {
        ASSERT_EQ(frames.size(), 1u);
        frame_start = frames[0].frame_start;
        frame_end = frames[0].frame_end;
        durations.assign(frames[0].nodes_durations.begin(), frames[0].nodes_durations.end());
    };

    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};
    Pipeline::Pipeline::Config config{};
    config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("first", "second");
    config.should_trace_execution = true;
    config.trace_flush_threshold = 1;
    ASSERT_TRUE(pipeline.configure(nodes, config, Pipeline::Pipeline::TraceCallback{on_trace}));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    pipeline.init();
    {
        ::testing::InSequence seq;
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{100}));
        EXPECT_CALL(first_node_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{130}));
        EXPECT_CALL(second_node_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{180}));
    }
    EXPECT_TRUE(pipeline.iterate());

    EXPECT_EQ(frame_start, Chrono::TimestampUS{100});
    EXPECT_EQ(frame_end, Chrono::TimestampUS{180});
    ASSERT_EQ(durations.size(), 2u);
    EXPECT_EQ(durations[0], Chrono::TimestampUS{30});
    EXPECT_EQ(durations[1], Chrono::TimestampUS{50});

    EXPECT_CALL(second_node_mock, release()).Times(1);
    EXPECT_CALL(first_node_mock, release()).Times(1);
    pipeline.release();
}
#else
TEST(PipelineTest, tracing_disabled_at_compile_time_is_a_warning)
{
    Pipeline::ClockMock clock_mock;
    Pipeline::IterableNodeMock<void> node_mock;
    Pipeline::Nodes nodes;
    nodes.add_node("node", node_mock);

    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};
    Pipeline::Pipeline::Config config{};
    config.order = makeVector<Pipeline::NodeName, DUCTA_MAX_NODES>("node");
    config.should_trace_execution = true;

    Configuration::Diagnostics diagnostics;
    ASSERT_TRUE(pipeline.configure(nodes, config, diagnostics));
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics[0].severity, Configuration::Severity::Warning);
}
#endif
