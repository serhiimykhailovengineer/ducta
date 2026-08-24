#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ducta/Pipeline/NodeRef.hpp"
#include "ducta/Pipeline/Pipeline.hpp"
#include "ducta/Pipeline/Clock.hpp"

#include "ducta/Pipeline/NodeMock.hpp"
#include "ducta/Pipeline/ClockMock.hpp"
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
    config.order = makeVector<StringView>("node_1", "node_2", "node_3", "node_4");

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

        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(node_2_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(node_3_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
        EXPECT_CALL(node_4_mock, iterate()).Times(1);
        EXPECT_CALL(clock_mock, now()).WillOnce(::testing::Return(Chrono::TimestampUS{0}));
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
    config.order = makeVector<StringView>("first", "second");

    ASSERT_TRUE(pipeline.configure(nodes, config));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    EXPECT_CALL(clock_mock, now()).WillRepeatedly(::testing::Return(Chrono::TimestampUS{0}));
    EXPECT_CALL(first_node_mock, iterate())
        .WillOnce(::testing::Return(Unexpected<Error>{Error{}}));
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
    config.order = makeVector<StringView>("first", "second");

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