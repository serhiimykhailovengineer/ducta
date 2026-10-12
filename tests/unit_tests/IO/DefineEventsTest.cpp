#include <gtest/gtest.h>

#include "ducta/IO/TEvent.hpp"
#include "ducta/IO/DefineEvents.hpp"

namespace {

struct EvA { int v; };
struct EvB { float v; };

struct EmittingNode
{
    ducta::IO::TEvent<EvA> ev_a;
    ducta::IO::TEvent<EvB> ev_b;
};

struct SilentNode {};

struct Receiver
{
    int calls = 0;
    void OnRaw(void*) { ++calls; }
};

} // namespace

DEFINE_NODE_EVENTS(EmittingNode, ("ev_a", ev_a), ("ev_b", ev_b));

TEST(DefineEventsTests, base_test)
{
    EmittingNode node;
    auto events = ducta::IO::NodeEventsTraits<EmittingNode>::get(node);

    EXPECT_EQ(events.size(), 2);
    ASSERT_TRUE(events.find("ev_a") != events.end());
    ASSERT_TRUE(events.find("ev_b") != events.end());
    EXPECT_EQ(events.find("ev_a")->second.type_id(), DUCTA_TYPE_ID(EvA));
    EXPECT_EQ(events.find("ev_b")->second.type_id(), DUCTA_TYPE_ID(EvB));
}

TEST(DefineEventsTests, ref_refers_to_node_member)
{
    EmittingNode node;
    auto events = ducta::IO::NodeEventsTraits<EmittingNode>::get(node);
    Receiver r;
    events.find("ev_a")->second.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));

    node.ev_a(EvA{1});
    node.ev_b(EvB{1.f});
    EXPECT_EQ(r.calls, 1);
}

TEST(DefineEventsTests, default_traits_is_empty)
{
    SilentNode node;
    auto events = ducta::IO::NodeEventsTraits<SilentNode>::get(node);
    EXPECT_EQ(events.size(), 0);
}
