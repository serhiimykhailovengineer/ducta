#include <gtest/gtest.h>

#include "ducta/IO/EventRef.hpp"
#include "ducta/IO/TEvent.hpp"

namespace {

struct EventA
{
    int value;
};
struct EventB
{
    float value;
};

struct Receiver
{
    int calls = 0;
    int last = 0;
    void OnRaw(void* raw)
    {
        ++calls;
        last = static_cast<EventA const*>(raw)->value;
    }
};

} // namespace

TEST(EventRefTests, type_id_matches_event_type)
{
    ducta::IO::TEvent<EventA> a;
    ducta::IO::TEvent<EventB> b;
    ducta::IO::EventRef ra{a};
    ducta::IO::EventRef rb{b};

    EXPECT_EQ(ra.type_id(), DUCTA_TYPE_ID(EventA));
    EXPECT_EQ(rb.type_id(), DUCTA_TYPE_ID(EventB));
    EXPECT_NE(ra.type_id(), rb.type_id());
}

TEST(EventRefTests, subscribe_through_ref_receives_events)
{
    ducta::IO::TEvent<EventA> event;
    ducta::IO::EventRef ref{event};
    Receiver r;

    ref.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));
    event(EventA{11});

    EXPECT_EQ(r.calls, 1);
    EXPECT_EQ(r.last, 11);
}

TEST(EventRefTests, unsubscribe_through_ref_stops_events)
{
    ducta::IO::TEvent<EventA> event;
    ducta::IO::EventRef ref{event};
    Receiver r;

    ref.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));
    ref.unsubscribe();
    event(EventA{1});

    EXPECT_EQ(r.calls, 0);
}

TEST(EventRefTests, copied_ref_controls_same_event)
{
    ducta::IO::TEvent<EventA> event;
    ducta::IO::EventRef ref{event};
    ducta::IO::EventRef copy = ref;
    Receiver r;

    copy.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));
    event(EventA{3});
    EXPECT_EQ(r.calls, 1);
}
