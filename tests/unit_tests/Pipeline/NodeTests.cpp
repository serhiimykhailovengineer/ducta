#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include "ducta/io/TInput.hpp"
#include "ducta/io/TOutput.hpp"

#include "ducta/ValueIO/ValueIOBuilder.hpp"

#include "ducta/Pipeline/Node.hpp"

class IterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, iterate, (), ());
    MOCK_METHOD(void, release, (), ());
};

class IterableNodeMockWrapper
{
public:
    std::reference_wrapper<IterableNodeMock> mock;

    void init() 
    {
        mock.get().init();
    }

    void iterate() 
    {
        mock.get().iterate();
    }

    void release() 
    {
        mock.get().release();
    }
};

class NonIterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, release, (), ());
};

class NonIterableNodeMockWrapper
{
public:
    std::reference_wrapper<NonIterableNodeMock> mock;

    void init() 
    {
        mock.get().init();
    }

    void release() 
    {
        mock.get().release();
    }
};

struct EmptyNode
{
    void init()
    {}

    void release()
    {}
};

using namespace ducta;

TEST(NodeTests, iterable_node_test) 
{
    IterableNodeMock node_mock;

    EXPECT_CALL(node_mock, init()).Times(1);
    EXPECT_CALL(node_mock, iterate()).Times(1);
    EXPECT_CALL(node_mock, release()).Times(1);

    Pipeline::Node node{IterableNodeMockWrapper{node_mock}};

    node.init();
    node.iterate();
    node.release();
}

TEST(NodeTests, non_iterable_node_test) 
{
    NonIterableNodeMock node_mock;

    EXPECT_CALL(node_mock, init()).Times(1);
    EXPECT_CALL(node_mock, release()).Times(1);

    Pipeline::Node node{NonIterableNodeMockWrapper{node_mock}};

    node.init();
    node.iterate();
    node.release();
}

TEST(NodeTests, iterable_non_copy_non_move_node_test) 
{
    struct NonCopyNonMoveNode
    {
        NonCopyNonMoveNode(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode(NonCopyNonMoveNode &&) noexcept = delete;

        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode &&) noexcept = delete;

        int a;
        float b;
        std::string c;

        void init()
        {}

        void iterate()
        {}

        void release()
        {}
    };

    Pipeline::Node node{std::in_place_type_t<NonCopyNonMoveNode>{}, 42, 54.4f, "yes"};
}

TEST(NodeTests, non_iterable_non_copy_non_move_node_test) 
{
    struct NonCopyNonMoveNode
    {
        NonCopyNonMoveNode(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode(NonCopyNonMoveNode &&) noexcept = delete;

        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode &&) noexcept = delete;

        int a;
        float b;
        std::string c;

        void init()
        {}

        void release()
        {}
    };

    Pipeline::Node node{std::in_place_type_t<NonCopyNonMoveNode>{}, 42, 54.4f, "yes"};
}


TEST(NodeTests, empty_inputs_and_outputs) 
{
    struct EmptyNode
    {
        void init()
        {}

        void release()
        {}
    };

    Pipeline::Node node{std::in_place_type_t<EmptyNode>{}};

    EXPECT_EQ(node.get_inputs().size(), 0U);
    EXPECT_EQ(node.get_outputs().size(), 0U);
}


struct NodeWithInputsAndOutputs
{
    IO::TInput<int> input1;
    IO::TInput<int> input2;
    IO::TOutput<int> output1;

    template <typename IOBuilder>
    NodeWithInputsAndOutputs(IOBuilder& builder)
    : input1{builder.template make_input<int>()}
    , input2{builder.template make_input<int>()}
    , output1{builder.template make_output<int>()}
    {}

    void init()
    {}

    void release()
    {}
};

namespace ducta {
namespace IO {
template<>
struct NodeInputsTraits<NodeWithInputsAndOutputs>
{
    using Node = NodeWithInputsAndOutputs;
    static std::map<std::string, IO::InputRef> get(Node& node)
    {
        return std::map<std::string, IO::InputRef>{
            {"input1", node.input1.get_ref()},
            {"input2", node.input2.get_ref()}
        };
    }
};

template<>
struct NodeOutputsTraits<NodeWithInputsAndOutputs>
{
    using Node = NodeWithInputsAndOutputs;
    static std::map<std::string, IO::OutputRef> get(Node& node)
    {
        return std::map<std::string, IO::OutputRef>{
            {"output1", node.output1.get_ref()}
        };
    }
};
} // namespace IO
} // namespace ducta

TEST(NodeTests, check_inputs_and_outputs) 
{
    IO::ValueIOBuilder io_builder;
    Pipeline::Node node{std::in_place_type_t<NodeWithInputsAndOutputs>{}, io_builder};

    auto inputs = node.get_inputs();
    ASSERT_EQ(inputs.size(), 2U);

    {
        auto it = inputs.find("input1");
        ASSERT_NE(it, inputs.end());
    }
    {
        auto it = inputs.find("input2");
        ASSERT_NE(it, inputs.end());
    }

    auto outputs = node.get_outputs();
    ASSERT_EQ(outputs.size(), 1U);

    {
        auto it = outputs.find("output1");
        ASSERT_NE(it, outputs.end());
    }
}