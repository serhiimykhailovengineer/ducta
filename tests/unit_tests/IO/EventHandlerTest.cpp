#include <gtest/gtest.h>

#include "ducta/IO/TEvent.hpp"
#include "ducta/IO/EventRef.hpp"
#include "ducta/IO/EventHandler.hpp"

namespace {

struct ExactEvent { int value; };

struct Consumer
{
    int calls = 0;
    int last = 0;
    ExactEvent const* last_ptr = nullptr;

    void OnExact(ExactEvent const& e)
    {
        ++calls;
        last = e.value;
        last_ptr = &e;
    }
};

using ExactHandler = ducta::IO::EventHandler<Consumer, &Consumer::OnExact>;

} // namespace

TEST(EventHandlerTests, deduces_event_type)
{
    static_assert(ducta::is_same_v<ExactHandler::EventType, ExactEvent>, "event type must be deduced");
    SUCCEED();
}

TEST(EventHandlerTests, typed_handler_receives_exact_event_from_tevent)
{
    ducta::IO::TEvent<ExactEvent> event;
    Consumer consumer;
    ExactHandler handler{consumer};

    event.subscribe(handler.delegate());

    ExactEvent e{42};
    event(e);

    EXPECT_EQ(consumer.calls, 1);
    EXPECT_EQ(consumer.last, 42);
    EXPECT_EQ(consumer.last_ptr, &e);
}

TEST(EventHandlerTests, typed_handler_via_event_ref)
{
    ducta::IO::TEvent<ExactEvent> event;
    ducta::IO::EventRef ref{event};
    Consumer consumer;
    ExactHandler handler{consumer};

    ref.subscribe(handler.delegate());
    event(ExactEvent{1});
    event(ExactEvent{2});

    EXPECT_EQ(consumer.calls, 2);
    EXPECT_EQ(consumer.last, 2);
}

TEST(EventHandlerTests, unsubscribe_stops_typed_handler)
{
    ducta::IO::TEvent<ExactEvent> event;
    ducta::IO::EventRef ref{event};
    Consumer consumer;
    ExactHandler handler{consumer};

    ref.subscribe(handler.delegate());
    ref.unsubscribe();
    event(ExactEvent{1});

    EXPECT_EQ(consumer.calls, 0);
}

TEST(EventHandlerTests, two_handlers_on_different_consumers)
{
    ducta::IO::TEvent<ExactEvent> event;
    Consumer a, b;
    ExactHandler ha{a}, hb{b};

    event.subscribe(ha.delegate());
    event(ExactEvent{1});
    event.subscribe(hb.delegate());
    event(ExactEvent{2});

    EXPECT_EQ(a.calls, 1);
    EXPECT_EQ(b.calls, 1);
    EXPECT_EQ(b.last, 2);
}
