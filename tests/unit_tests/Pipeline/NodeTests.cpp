#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"

#include "ducta/Pipeline/NodeRef.hpp"

#include "ducta/Core/Types/ReferenceWrapper.hpp"

class IterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, iterate, (), ());
    MOCK_METHOD(void, release, (), ());
};

class NonIterableNodeMock
{
public:
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, release, (), ());
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

    Pipeline::NodeRef node{node_mock};

    node.init();
    node.iterate();
    node.release();
}

TEST(NodeTests, non_iterable_node_test) 
{
    NonIterableNodeMock node_mock;

    EXPECT_CALL(node_mock, init()).Times(1);
    EXPECT_CALL(node_mock, release()).Times(1);

    Pipeline::NodeRef node_ref{node_mock};

    node_ref.init();
    node_ref.iterate();
    node_ref.release();
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
    } node_instance{42, 54.4f, "yes"};

    Pipeline::NodeRef node_ref{node_instance};
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

    NonCopyNonMoveNode node{42, 54.4f, "yes"};

    Pipeline::NodeRef node_ref{node};
}


TEST(NodeTests, empty_inputs_and_outputs) 
{
    struct EmptyNode
    {
        void init()
        {}

        void release()
        {}
    } empty_node;

    Pipeline::NodeRef node_ref{empty_node};

    EXPECT_EQ(node_ref.get_inputs().size(), 0U);
    EXPECT_EQ(node_ref.get_outputs().size(), 0U);
}


struct NodeWithInputsAndOutputs
{
    IO::TInput<int> input1;
    IO::TInput<int> input2;
    IO::TOutput<int> output1;

    NodeWithInputsAndOutputs()
    {}

    void init()
    {}

    void release()
    {}
};

DEFINE_NODE_INPUTS(NodeWithInputsAndOutputs, 
        ("input1", input1),
        ("input2", input2)
    );

DEFINE_NODE_OUTPUTS(NodeWithInputsAndOutputs, 
        ("output1", output1)
    );

TEST(NodeTests, check_inputs_and_outputs) 
{
    NodeWithInputsAndOutputs node_instance;
    Pipeline::NodeRef node_ref{node_instance};

    auto inputs = node_ref.get_inputs();
    ASSERT_EQ(inputs.size(), 2U);

    {
        auto it = inputs.find("input1");
        ASSERT_NE(it, inputs.end());
    }
    {
        auto it = inputs.find("input2");
        ASSERT_NE(it, inputs.end());
    }

    auto outputs = node_ref.get_outputs();
    ASSERT_EQ(outputs.size(), 1U);

    {
        auto it = outputs.find("output1");
        ASSERT_NE(it, outputs.end());
    }
}

TEST(NodeTests, check_result_of_iterate) 
{
    struct NodeWithBoolIterate
    {
        void init()
        {}

        bool iterate()
        {
            return false;
        }

        void release()
        {}
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    auto result = node_ref.iterate();
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
    node_ref.release();
}

TEST(NodeTests, check_result_of_iterate_with_expected) 
{
    struct NodeWithExpectedIterate
    {
        using value_type = bool;
        using error_type = Error;

        void init()
        {}

        Expected<bool, Error> iterate()
        {
            return false;
        }

        void release()
        {}
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    auto result = node_ref.iterate();
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
    node_ref.release();
}

TEST(NodeTests, check_result_of_iterate_with_expected_error) 
{
    struct NodeWithExpectedIterate
    {
        using value_type = bool;
        using error_type = Error;

        void init()
        {}

        Expected<bool, Error> iterate()
        {
            return Unexpected<Error>{Error{}};
        }

        void release()
        {}
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    auto result = node_ref.iterate();
    ASSERT_FALSE(result);
    node_ref.release();
}

TEST(NodeTests, check_result_of_custom_expected_like_iterate) 
{
    struct CustomExpectedLike
    {
        using value_type = bool;
        using error_type = Error;

        CustomExpectedLike(bool has_value, bool value = false)
        : m_has_value{has_value}, m_value{value}
        {}

        explicit operator bool() const
        {
            return m_has_value;
        }

        bool operator*() const
        {
            return m_value;
        }

        Error error() const
        {
            return {};
        }

    private:
        bool m_has_value;
        bool m_value;
    };

    struct NodeWithCustomExpectedLikeIterate
    {
        void init()
        {}

        CustomExpectedLike iterate()
        {
            return CustomExpectedLike{true, false};
        }

        void release()
        {}
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    auto result = node_ref.iterate();
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
    node_ref.release();
}