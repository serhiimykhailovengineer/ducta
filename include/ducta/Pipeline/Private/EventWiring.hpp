#ifndef DUCTA_PIPELINE_PRIVATE_EVENTWIRING_HPP
#define DUCTA_PIPELINE_PRIVATE_EVENTWIRING_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/Diagnostics.hpp"

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Pipeline/Private/PipelineTraceInfo.hpp"

#include "ducta/IO/EventRef.hpp"

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/TypeRegistry.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

/* false while a fixed capacity container (embedded backend) can take another element */
template <class Container>
bool is_full(Container const& container) noexcept
{
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    return container.full();
#else
    (void)container;
    return false;
#endif
}

/*
 * Connects event emitters (DEFINE_NODE_EVENTS) with event receivers
 * (SupportedEvents + OnEvent) of the nodes of a pipeline.
 * A node which both emits and supports an event type receives its own events.
 * With tracing enabled every delivery is recorded in PipelineTraceInfo.
 *
 * Every emitter (TEvent) has a single delegate slot, so each emitter gets one
 * Fanout that is subscribed to it and forwards the event to every node which
 * supports exactly that event type.
 *
 * With the embedded backend at most DUCTA_MAX_EVENT_CONNECTIONS emitters are wired, each to at
 * most DUCTA_MAX_EVENT_CONNECTIONS receivers; a graph needing more is rejected by connect().
 */
class EventWiring
{
public:
    struct Sink
    {
        NodeRef node;
        ::ducta::Private::TypeId id;
        StringView name;
        size_t index;
    };

    class Fanout
    {
    public:
        Fanout(
            IO::EventRef emitter,
            StringView emitter_name,
            size_t emitter_index,
            Clock* clock,
            PipelineTraceInfo* trace)
            : m_emitter(emitter)
            , m_emitter_name(emitter_name)
            , m_emitter_index(emitter_index)
            , m_clock(clock)
            , m_trace(trace)
        {
        }

        /* false if the sink does not fit */
        bool add_sink(Sink sink)
        {
            if (is_full(m_sinks))
            {
                return false;
            }
            m_sinks.push_back(sink);
            return true;
        }
        size_t sink_count() const
        {
            return m_sinks.size();
        }

        void dispatch(void* event)
        {
            for (auto& sink : m_sinks)
            {
                bool const traced = m_trace && m_clock && m_trace->frame_open();
                Chrono::TimestampUS const start = traced ? m_clock->now() : Chrono::TimestampUS{0};

                auto result = sink.node.sendEventRaw(sink.id, event);

                if (traced)
                {
                    m_trace->event(
                        EventTraceInfo{
                            m_emitter_index, sink.index, sink.id, start, m_clock->now() - start});
                }
                if (!result)
                {
                    DUCTA_LOG_ERROR("Event from '{}' failed in node '{}'", m_emitter_name, sink.name);
                }
            }
        }

        void subscribe()
        {
            m_emitter.subscribe(::ducta::makeFunctionRef<Fanout, &Fanout::dispatch>(*this));
        }

        void unsubscribe()
        {
            m_emitter.unsubscribe();
        }

    private:
        IO::EventRef m_emitter;
        StringView m_emitter_name;
        size_t m_emitter_index;
        Clock* m_clock;
        PipelineTraceInfo* m_trace;
        Vector<Sink, DUCTA_MAX_EVENT_CONNECTIONS> m_sinks;
    };

    /*
     * @param nodes  nodes in pipeline order (the index is the index used in the trace)
     * @param clock  used for timestamps of traced events (may be null)
     * @param trace  trace sink, null when tracing is disabled
     * @param diagnostics  receives the problems; null: they are logged
     * @return false (nothing connected) if the events exceed DUCTA_MAX_EVENT_CONNECTIONS
     */
    bool connect(
        Vector<NodeInfo, DUCTA_MAX_NODES>& nodes,
        Clock* clock = nullptr,
        PipelineTraceInfo* trace = nullptr,
        Configuration::Diagnostics* diagnostics = nullptr)
    {
        disconnect();

        // Phase 1: build all fanouts. Nothing refers to them yet, so the storage may move.
        for (size_t emitter_idx = 0; emitter_idx < nodes.size(); ++emitter_idx)
        {
            auto events = nodes[emitter_idx].node.get_events();
            for (auto& event : events)
            {
                IO::EventRef emitter = event.second;
                auto const id = emitter.type_id();

                Fanout fanout{emitter, nodes[emitter_idx].name, emitter_idx, clock, trace};
                for (size_t receiver_idx = 0; receiver_idx < nodes.size(); ++receiver_idx)
                {
                    for (auto supported : nodes[receiver_idx].node.get_supported_event_ids())
                    {
                        if (supported == id)
                        {
                            if (!fanout.add_sink(Sink{
                                    nodes[receiver_idx].node, id, nodes[receiver_idx].name, receiver_idx}))
                            {
                                String<100> text;
                                format_to(
                                    text,
                                    "event '{}' of node '{}' has more than {} receivers",
                                    event.first,
                                    nodes[emitter_idx].name,
                                    static_cast<unsigned long long>(DUCTA_MAX_EVENT_CONNECTIONS));
                                return fail(diagnostics, text);
                            }
                            break;
                        }
                    }
                }
                if (fanout.sink_count() > 0)
                {
                    if (is_full(m_fanouts))
                    {
                        String<100> text;
                        format_to(
                            text,
                            "more than {} connected event emitters; event '{}' of node '{}' does "
                            "not fit",
                            static_cast<unsigned long long>(DUCTA_MAX_EVENT_CONNECTIONS),
                            event.first,
                            nodes[emitter_idx].name);
                        return fail(diagnostics, text);
                    }
                    m_fanouts.push_back(::ducta::move(fanout));
                }
            }
        }

        // Phase 2: addresses are final, hand them out to the emitters.
        for (auto& fanout : m_fanouts)
        {
            fanout.subscribe();
        }
        return true;
    }

    void disconnect()
    {
        for (auto& fanout : m_fanouts)
        {
            fanout.unsubscribe();
        }
        m_fanouts.clear();
    }

    size_t connection_count() const
    {
        return m_fanouts.size();
    }

    EventWiring() = default;
    EventWiring(EventWiring const&) = delete;
    EventWiring& operator=(EventWiring const&) = delete;
    ~EventWiring()
    {
        disconnect();
    }

private:
    /* Report a capacity problem and drop what was built so far; always false */
    bool fail(Configuration::Diagnostics* diagnostics, String<100> const& text)
    {
        m_fanouts.clear();
        if (diagnostics != nullptr)
        {
            diagnostics->error(Configuration::PropNode{}, StringView{text.data(), text.size()});
        }
        else
        {
            DUCTA_LOG_ERROR("{}", StringView{text.data(), text.size()});
        }
        return false;
    }

    Vector<Fanout, DUCTA_MAX_EVENT_CONNECTIONS> m_fanouts;
};

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_EVENTWIRING_HPP
