#include <gtest/gtest.h>

#include "ducta/Core/Types/FunctionRef.hpp"
#include "ducta/IO/TEvent.hpp"

namespace {

struct TestEvent
{
    int value;
};

struct Receiver
{
    int calls = 0;
    int last_value = 0;
    TestEvent const* last_ptr = nullptr;

    void OnRaw(void* raw)
    {
        auto const* ev = static_cast<TestEvent const*>(raw);
        ++calls;
        last_ptr = ev;
        last_value = ev->value;
    }
};

} // namespace

TEST(TEventTests, emit_without_subscriber_is_noop)
{
    ducta::IO::TEvent<TestEvent> event;
    TestEvent e{1};
    EXPECT_NO_THROW(event(e));
}

TEST(TEventTests, subscribe_and_emit_delivers_payload)
{
    ducta::IO::TEvent<TestEvent> event;
    Receiver r;
    event.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));

    TestEvent e{42};
    event(e);

    EXPECT_EQ(r.calls, 1);
    EXPECT_EQ(r.last_value, 42);
    EXPECT_EQ(r.last_ptr, &e);
}

TEST(TEventTests, emit_multiple_times)
{
    ducta::IO::TEvent<TestEvent> event;
    Receiver r;
    event.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));

    for (int i = 0; i < 3; ++i)
    {
        TestEvent e{i};
        event(e);
    }
    EXPECT_EQ(r.calls, 3);
    EXPECT_EQ(r.last_value, 2);
}

TEST(TEventTests, unsubscribe_stops_delivery)
{
    ducta::IO::TEvent<TestEvent> event;
    Receiver r;
    event.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(r));
    event.unsubscribe();

    TestEvent e{5};
    event(e);
    EXPECT_EQ(r.calls, 0);
}

TEST(TEventTests, resubscribe_replaces_delegate)
{
    ducta::IO::TEvent<TestEvent> event;
    Receiver a, b;
    event.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(a));
    event.subscribe(ducta::makeFunctionRef<Receiver, &Receiver::OnRaw>(b));

    TestEvent e{7};
    event(e);
    EXPECT_EQ(a.calls, 0);
    EXPECT_EQ(b.calls, 1);
}
