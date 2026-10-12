#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include "ducta/IO/DefineEvents.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/EventHandler.hpp"
#include "ducta/IO/TEvent.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"

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
    {
    }

    void release()
    {
    }
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
        NonCopyNonMoveNode(NonCopyNonMoveNode&&) noexcept = delete;

        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode&&) noexcept = delete;

        int a;
        float b;
        std::string c;

        void init()
        {
        }

        void iterate()
        {
        }

        void release()
        {
        }
    } node_instance{42, 54.4f, "yes"};

    Pipeline::NodeRef node_ref{node_instance};
}

TEST(NodeTests, non_iterable_non_copy_non_move_node_test)
{
    struct NonCopyNonMoveNode
    {
        NonCopyNonMoveNode(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode(NonCopyNonMoveNode&&) noexcept = delete;

        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode const&) = delete;
        NonCopyNonMoveNode& operator=(NonCopyNonMoveNode&&) noexcept = delete;

        int a;
        float b;
        std::string c;

        void init()
        {
        }

        void release()
        {
        }
    };

    NonCopyNonMoveNode node{42, 54.4f, "yes"};

    Pipeline::NodeRef node_ref{node};
}

TEST(NodeTests, empty_inputs_and_outputs)
{
    struct EmptyNode
    {
        void init()
        {
        }

        void release()
        {
        }
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
    {
    }

    void init()
    {
    }

    void release()
    {
    }
};

DEFINE_NODE_INPUTS(NodeWithInputsAndOutputs, ("input1", input1), ("input2", input2));

DEFINE_NODE_OUTPUTS(NodeWithInputsAndOutputs, ("output1", output1));

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
        {
        }

        bool iterate()
        {
            return false;
        }

        void release()
        {
        }
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
        {
        }

        Expected<bool, Error> iterate()
        {
            return false;
        }

        void release()
        {
        }
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
        {
        }

        Expected<bool, Error> iterate()
        {
            return Unexpected<Error>{Error{}};
        }

        void release()
        {
        }
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
            : m_has_value{has_value}
            , m_value{value}
        {
        }

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
        {
        }

        CustomExpectedLike iterate()
        {
            return CustomExpectedLike{true, false};
        }

        void release()
        {
        }
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    auto result = node_ref.iterate();
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
    node_ref.release();
}

TEST(NodeTests, send_event_to_node_with_supported_event)
{
    struct MyEvent
    {
        int value;
    };

    struct NodeWithSupportedEvent
    {
        using SupportedEvents = EventList<MyEvent>;

        void init()
        {
        }

        void OnEvent(MyEvent const& event)
        {
            m_last_event_value = event.value;
        }

        void release()
        {
        }

        int m_last_event_value{0};
    } node_instance;

    Pipeline::NodeRef node_ref{node_instance};

    node_ref.init();
    MyEvent event{42};
    auto result = node_ref.sendEvent(event);
    ASSERT_TRUE(result);
    ASSERT_TRUE(*result);
    ASSERT_EQ(node_instance.m_last_event_value, 42);
    node_ref.release();
}

namespace {

struct PingEvent
{
    int value;
};

struct EmitterNode
{
    ducta::IO::TEvent<PingEvent> ping;
    void init()
    {
    }
    void release()
    {
    }
    void fire(int v)
    {
        ping(PingEvent{v});
    }
};

struct PingReceiver
{
    int calls = 0;
    int last = 0;
    void OnRaw(void* raw)
    {
        ++calls;
        last = static_cast<PingEvent const*>(raw)->value;
    }
};

} // namespace

DEFINE_NODE_EVENTS(EmitterNode, ("ping", ping));

TEST(NodeTests, get_events_for_node_without_events)
{
    EmptyNode empty;
    Pipeline::NodeRef node{empty};
    EXPECT_EQ(node.get_events().size(), 0);
}

TEST(NodeTests, get_events_for_node_with_events)
{
    EmitterNode emitter;
    Pipeline::NodeRef node{emitter};

    auto events = node.get_events();
    ASSERT_EQ(events.size(), 1);
    ASSERT_TRUE(events.find("ping") != events.end());
    EXPECT_EQ(events.find("ping")->second.type_id(), DUCTA_TYPE_ID(PingEvent));
}

TEST(NodeTests, emitted_event_reaches_subscriber_via_node_ref)
{
    EmitterNode emitter;
    PingReceiver receiver;
    Pipeline::NodeRef node{emitter};

    auto events = node.get_events();
    events.find("ping")->second.subscribe(
        makeFunctionRef<PingReceiver, &PingReceiver::OnRaw>(receiver));

    emitter.fire(1);
    emitter.fire(2);

    EXPECT_EQ(receiver.calls, 2);
    EXPECT_EQ(receiver.last, 2);
}

namespace {
struct PingConsumer
{
    int calls = 0;
    int last = 0;
    void OnExact(PingEvent const& e)
    {
        ++calls;
        last = e.value;
    }
};
} // namespace

TEST(NodeTests, emitted_event_reaches_exact_typed_handler_via_node_ref)
{
    EmitterNode emitter;
    PingConsumer consumer;
    IO::EventHandler<PingConsumer, &PingConsumer::OnExact> handler{consumer};
    Pipeline::NodeRef node{emitter};

    node.get_events().find("ping")->second.subscribe(handler.delegate());

    emitter.fire(5);

    EXPECT_EQ(consumer.calls, 1);
    EXPECT_EQ(consumer.last, 5);
}
