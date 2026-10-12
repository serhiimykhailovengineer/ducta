#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <initializer_list>
#include <vector>

#include "ducta/IO/DefineEvents.hpp"
#include "ducta/IO/TEvent.hpp"

#include "ducta/Pipeline/ClockMock.hpp"
#include "ducta/Pipeline/Nodes.hpp"
#include "ducta/Pipeline/Pipeline.hpp"

using namespace ducta;

namespace {

struct Ping
{
    int value;
};
struct Pong
{
    int value;
};

struct Emitter
{
    IO::TEvent<Ping> ping;
    IO::TEvent<Pong> pong;
    void init()
    {
    }
    void release()
    {
    }
};

struct PingReceiver
{
    using SupportedEvents = EventList<Ping>;
    int calls = 0;
    int last = 0;
    void init()
    {
    }
    void release()
    {
    }
    void OnEvent(Ping const& e)
    {
        ++calls;
        last = e.value;
    }
};

struct PingPongReceiver
{
    using SupportedEvents = EventList<Ping, Pong>;
    int pings = 0;
    int pongs = 0;
    void init()
    {
    }
    void release()
    {
    }
    void OnEvent(Ping const&)
    {
        ++pings;
    }
    void OnEvent(Pong const&)
    {
        ++pongs;
    }
};

// Receives Ping and emits Pong in response (receiver + emitter in one node).
struct Relay
{
    using SupportedEvents = EventList<Ping>;
    IO::TEvent<Pong> pong;
    void init()
    {
    }
    void release()
    {
    }
    void OnEvent(Ping const& e)
    {
        pong(Pong{e.value + 1});
    }
};

// Emits Ping from iterate().
struct Producer
{
    IO::TEvent<Ping> ping;
    void init()
    {
    }
    void release()
    {
    }
    void iterate()
    {
        ping(Ping{1});
    }
};

// Handles the very event it emits.
struct SelfListener
{
    using SupportedEvents = EventList<Pong>;
    IO::TEvent<Pong> pong;
    int calls = 0;
    void init()
    {
    }
    void release()
    {
    }
    void OnEvent(Pong const&)
    {
        ++calls;
    }
};

struct TickClock
{
    Chrono::TimestampUS t{0};
    Chrono::TimestampUS now()
    {
        t += Chrono::TimestampUS{10};
        return t;
    }
    Chrono::TimestampUS epoch()
    {
        return Chrono::TimestampUS{0};
    }
    void sleepFor(Chrono::TimestampUS)
    {
    }
};

struct Silent
{
    void init()
    {
    }
    void release()
    {
    }
};

} // namespace

DEFINE_NODE_EVENTS(Emitter, ("ping", ping), ("pong", pong));
DEFINE_NODE_EVENTS(Relay, ("pong", pong));
DEFINE_NODE_EVENTS(Producer, ("ping", ping));
DEFINE_NODE_EVENTS(SelfListener, ("pong", pong));

namespace {

struct Fixture : ::testing::Test
{
    // Nodes are declared before the pipeline: they must outlive it,
    // the pipeline unsubscribes from emitters in its destructor.
    Emitter e;
    PingReceiver r, a, b, in, out;
    PingPongReceiver sink;
    Relay relay;
    Silent silent;
    Producer producer;
    SelfListener self;

    Pipeline::ClockMock clock;
    Pipeline::Nodes nodes;
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock}};

    bool configure(std::initializer_list<StringView> order)
    {
        Pipeline::Pipeline::Config config{};
        for (auto name : order)
            config.order.push_back(Pipeline::NodeName{name.data(), name.size()});
        return pipeline.configure(nodes, config);
    }
};

} // namespace

TEST_F(Fixture, supported_event_ids_are_exposed)
{
    PingPongReceiver r;
    Silent s;
    Pipeline::NodeRef rr{r};
    Pipeline::NodeRef ss{s};

    auto ids = rr.get_supported_event_ids();
    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], DUCTA_TYPE_ID(Ping));
    EXPECT_EQ(ids[1], DUCTA_TYPE_ID(Pong));
    EXPECT_EQ(ss.get_supported_event_ids().size(), 0);
}

TEST_F(Fixture, event_is_delivered_to_receiver_after_configure)
{
    nodes.add_node("emitter", e);
    nodes.add_node("receiver", r);
    ASSERT_TRUE(configure({"emitter", "receiver"}));

    e.ping(Ping{7});
    EXPECT_EQ(r.calls, 1);
    EXPECT_EQ(r.last, 7);
}

TEST_F(Fixture, event_is_broadcast_to_all_receivers_of_that_type)
{
    nodes.add_node("emitter", e);
    nodes.add_node("a", a);
    nodes.add_node("b", sink);
    ASSERT_TRUE(configure({"emitter", "a", "b"}));

    e.ping(Ping{1});
    e.ping(Ping{2});
    e.pong(Pong{3});

    EXPECT_EQ(a.calls, 2);
    EXPECT_EQ(sink.pings, 2);
    EXPECT_EQ(sink.pongs, 1);
}

TEST_F(Fixture, receivers_get_only_exact_event_type)
{
    nodes.add_node("emitter", e);
    nodes.add_node("receiver", r);
    ASSERT_TRUE(configure({"emitter", "receiver"}));

    e.pong(Pong{1});
    EXPECT_EQ(r.calls, 0);
}

TEST_F(Fixture, node_not_in_order_is_not_connected)
{
    nodes.add_node("emitter", e);
    nodes.add_node("in", in);
    nodes.add_node("out", out);
    ASSERT_TRUE(configure({"emitter", "in"}));

    e.ping(Ping{1});
    EXPECT_EQ(in.calls, 1);
    EXPECT_EQ(out.calls, 0);
}

TEST_F(Fixture, emitter_without_receivers_is_noop)
{
    nodes.add_node("emitter", e);
    nodes.add_node("silent", silent);
    ASSERT_TRUE(configure({"emitter", "silent"}));

    EXPECT_NO_THROW(e.ping(Ping{1}));
}

TEST_F(Fixture, relay_receives_and_emits_chained_events)
{
    nodes.add_node("emitter", e);
    nodes.add_node("relay", relay);
    nodes.add_node("sink", sink);
    ASSERT_TRUE(configure({"emitter", "relay", "sink"}));

    // emitter -> relay (Ping) -> emits Pong -> sink (Pong)
    e.ping(Ping{1});
    EXPECT_EQ(sink.pings, 1);
    EXPECT_EQ(sink.pongs, 1);
}

TEST_F(Fixture, reconfigure_replaces_connections)
{
    nodes.add_node("emitter", e);
    nodes.add_node("a", a);
    nodes.add_node("b", b);

    ASSERT_TRUE(configure({"emitter", "a"}));
    ASSERT_TRUE(configure({"emitter", "b"}));

    e.ping(Ping{1});
    EXPECT_EQ(a.calls, 0);
    EXPECT_EQ(b.calls, 1);
}

TEST_F(Fixture, release_disconnects_events)
{
    nodes.add_node("emitter", e);
    nodes.add_node("receiver", r);
    ASSERT_TRUE(configure({"emitter", "receiver"}));

    pipeline.release();
    e.ping(Ping{1});
    EXPECT_EQ(r.calls, 0);
}

TEST_F(Fixture, node_receives_its_own_event_if_it_has_a_handler)
{
    nodes.add_node("self", self);
    ASSERT_TRUE(configure({"self"}));

    self.pong(Pong{1});
    EXPECT_EQ(self.calls, 1);
}

namespace {

struct TracedFixture : ::testing::Test
{
    Producer producer;
    PingReceiver receiver;
    TickClock clock;
    Pipeline::Nodes nodes;
    Pipeline::Pipeline pipeline{Pipeline::Clock{clock}};

    struct Frame
    {
        std::vector<Pipeline::EventTraceInfo> events;
    };
    std::vector<Frame> frames;
    size_t name_count = 0;

    // Functor kept as a member: FunctionRef (etl::delegate) does not own it.
    struct Collector
    {
        TracedFixture* fixture;
        void operator()(Span<StringView const> names, Span<Pipeline::FrameTraceInfo const> infos) const
        {
            fixture->name_count = names.size();
            for (auto const& info : infos)
            {
                Frame f;
                for (auto const& e : info.events) f.events.push_back(e);
                fixture->frames.push_back(f);
            }
        }
    };
    Collector collect{this};

    bool configure(bool trace)
    {
        nodes.add_node("producer", producer);
        nodes.add_node("receiver", receiver);

        Pipeline::Pipeline::Config config{};
        config.order.push_back("producer");
        config.order.push_back("receiver");
        config.should_trace_execution = trace;
        config.trace_flush_threshold = 1;

        return pipeline.configure(nodes, config, Pipeline::Pipeline::TraceCallback{collect});
    }
};

} // namespace

TEST_F(TracedFixture, delivered_event_is_recorded_in_frame_trace)
{
    ASSERT_TRUE(configure(true));

    ASSERT_TRUE(pipeline.iterate());

    EXPECT_EQ(receiver.calls, 1);
    ASSERT_EQ(frames.size(), 1);
    ASSERT_EQ(frames[0].events.size(), 1);
    auto const& e = frames[0].events[0];
    EXPECT_EQ(name_count, 2);
    EXPECT_EQ(e.emitter_index, 0);
    EXPECT_EQ(e.receiver_index, 1);
    EXPECT_EQ(e.event_id, DUCTA_TYPE_ID(Ping));
    EXPECT_GT(e.duration.count(), 0);
    EXPECT_GT(e.timestamp.count(), 0);
}

TEST_F(TracedFixture, event_name_can_be_resolved_from_trace)
{
    ASSERT_TRUE(configure(true));
    ASSERT_TRUE(pipeline.iterate());

    auto name = ::ducta::Private::TypeRegistry::instance().name_of(frames.at(0).events.at(0).event_id);
    EXPECT_FALSE(name.empty());
}

TEST_F(TracedFixture, each_frame_has_its_own_events)
{
    ASSERT_TRUE(configure(true));
    ASSERT_TRUE(pipeline.iterate());
    ASSERT_TRUE(pipeline.iterate());

    ASSERT_EQ(frames.size(), 2);
    EXPECT_EQ(frames[0].events.size(), 1);
    EXPECT_EQ(frames[1].events.size(), 1);
}

TEST_F(TracedFixture, events_are_delivered_but_not_traced_when_tracing_is_off)
{
    ASSERT_TRUE(configure(false));
    ASSERT_TRUE(pipeline.iterate());

    EXPECT_EQ(receiver.calls, 1);
    EXPECT_TRUE(frames.empty());
}

TEST_F(TracedFixture, event_emitted_outside_of_frame_is_not_traced)
{
    ASSERT_TRUE(configure(true));

    producer.ping(Ping{5}); // no frame is open
    EXPECT_EQ(receiver.calls, 1);

    ASSERT_TRUE(pipeline.iterate());
    ASSERT_EQ(frames.size(), 1);
    EXPECT_EQ(frames[0].events.size(), 1); // only the one from the iterate()
}
