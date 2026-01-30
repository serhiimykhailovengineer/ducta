#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ducta/Pipeline/Node.hpp"
#include "ducta/Pipeline/Pipeline.hpp"
#include "ducta/Pipeline/Clock.hpp"

#include "ducta/Pipeline/NodeMock.hpp"
#include "ducta/Pipeline/ClockMock.hpp"

using namespace ducta;

TEST(PipelineTest, base_test) 
{
    Pipeline::ClockMock clock_mock;

    Pipeline::NonIterableNodeMock node_1_mock;
    Pipeline::IterableNodeMock<void> node_2_mock;
    Pipeline::IterableNodeMock<void> node_3_mock;
    Pipeline::IterableNodeMock<void> node_4_mock;

    // Create nodes map
    std::map<std::string, Pipeline::Node> nodes{};

    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("node_1"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::NonIterableNodeMockWrapper>{}, node_1_mock));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("node_2"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::IterableNodeMockWrapper<void>>{}, node_2_mock));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("node_3"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::IterableNodeMockWrapper<void>>{}, node_3_mock));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("node_4"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::IterableNodeMockWrapper<void>>{}, node_4_mock));

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    pipeline.configure(
        nodes,
        {"node_1", "node_2", "node_3", "node_4"},
        {"node_2", "node_3", "node_4"}
    );

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
    std::map<std::string, Pipeline::Node> nodes{};

    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("first"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::IterableNodeMockWrapper<Expected<void, Error>>>{}, first_node_mock));
    nodes.emplace(std::piecewise_construct, 
                  std::forward_as_tuple("second"), 
                  std::forward_as_tuple(std::in_place_type_t<Pipeline::IterableNodeMockWrapper<Expected<void, Error>>>{}, second_node_mock));

    // Configure pipeline
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock_mock}};

    ASSERT_TRUE(pipeline.configure(
        nodes,
        {"first", "second"}
    ));

    EXPECT_CALL(first_node_mock, init()).Times(1);
    EXPECT_CALL(second_node_mock, init()).Times(1);
    EXPECT_CALL(first_node_mock, iterate())
        .WillOnce(::testing::Return(Unexpected<Error>{Error{}}));
    EXPECT_CALL(second_node_mock, iterate()).Times(0);
    EXPECT_CALL(second_node_mock, release()).Times(1);
    EXPECT_CALL(first_node_mock, release()).Times(1);

    pipeline.init();
    EXPECT_FALSE(pipeline.iterate());
    pipeline.release();
}