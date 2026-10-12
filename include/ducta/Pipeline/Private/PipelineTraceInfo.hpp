#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP
#define DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Chrono/Chrono.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"

#include "ducta/Core/Assert.hpp"
#include "ducta/Core/Types/Span.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/TypeRegistry.hpp"
#include "ducta/Core/Types/Vector.hpp"

namespace ducta {
namespace Pipeline {

/* Capacities of the trace buffers (fixed-size containers on the embedded backend). */
constexpr size_t MAX_TRACED_NODES = DUCTA_MAX_NODES;
constexpr size_t MAX_TRACED_EVENTS_PER_FRAME = DUCTA_MAX_TRACED_EVENTS_PER_FRAME;
constexpr size_t MAX_TRACED_FRAMES = DUCTA_MAX_TRACED_FRAMES;

/*
 * One delivery of an event to one receiver.
 * The handler runs inside the emitter's iterate(), so `duration` is already
 * part of the emitter's entry in nodes_durations (not additional time).
 * Indices refer to the node_names passed to the trace callback.
 * Event name: ::ducta::Private::TypeRegistry::instance().name_of(event_id).
 */
struct EventTraceInfo
{
    size_t emitter_index;
    size_t receiver_index;
    ::ducta::Private::TypeId event_id;
    Chrono::TimestampUS timestamp; // delivery start
    Chrono::TimestampUS duration; // time spent in the receiver's OnEvent
};

struct FrameTraceInfo
{
    size_t frame_index;
    Chrono::TimestampUS frame_start;
    Chrono::TimestampUS frame_end;
    Vector<Chrono::TimestampUS, MAX_TRACED_NODES> nodes_durations;
    Vector<EventTraceInfo, MAX_TRACED_EVENTS_PER_FRAME> events;
};

namespace Private {

class PipelineTraceInfo
{
public:
    /* nodes: the traced nodes in pipeline order; they must outlive the trace configuration */
    void configure(Span<NodeInfo const> const& nodes)
    {
        m_nodes = nodes;
        m_frames_info.clear();
        m_dropped_frames = 0;
        m_current_frame_index = 0;
    }

    void frame_start(Chrono::TimestampUS ts)
    {
        DUCTA_ASSERT(!m_frame_open);

        /* Without a flush (no callback, or a threshold above the capacity) keep the latest
         * frames: drop the oldest one */
        if (m_frames_info.size() >= MAX_TRACED_FRAMES)
        {
            m_frames_info.erase(m_frames_info.begin());
            ++m_dropped_frames;
        }
        m_frames_info.emplace_back();

        auto& current_frame_info = m_frames_info.back();
        current_frame_info.frame_index = m_current_frame_index++;
        current_frame_info.nodes_durations.resize(m_nodes.size(), Chrono::TimestampUS{0});
        current_frame_info.frame_start = ts;
        current_frame_info.events.clear();

        m_frame_open = true;
    }

    void frame_end(Chrono::TimestampUS ts)
    {
        DUCTA_ASSERT(m_frame_open);

        auto& current_frame_info = m_frames_info.back();
        current_frame_info.frame_end = ts;
        m_frame_open = false;
    }

    void node_duration(size_t node_index, Chrono::TimestampUS ts)
    {
        DUCTA_ASSERT(m_frame_open);

        auto& current_frame_info = m_frames_info.back();

        DUCTA_ASSERT(node_index < m_nodes.size());
        current_frame_info.nodes_durations[node_index] = ts;
    }

    bool frame_open() const
    {
        return m_frame_open;
    }

    /* Events emitted outside of a frame (init, other threads) are not traced. */
    void event(EventTraceInfo const& info)
    {
        if (!m_frame_open)
        {
            return;
        }

        auto& events = m_frames_info.back().events;
        if (events.size() >= events.max_size())
        {
            return;
        }
        events.push_back(info);
    }

    template <typename Callback>
    void flush(Callback&& callback)
    {
        if (m_frames_info.empty())
        {
            return;
        }

        Vector<StringView, MAX_TRACED_NODES> node_names;
        for (NodeInfo const& node : m_nodes)
        {
            node_names.push_back(node.name);
        }
        callback(
            Span<StringView const>(node_names.data(), node_names.size()),
            Span<FrameTraceInfo const>(m_frames_info.data(), m_frames_info.size()));
        m_frames_info.clear();
    }

    size_t pending_frames() const
    {
        return m_frames_info.size();
    }

    /* Frames dropped because the buffer was full */
    size_t dropped_frames() const
    {
        return m_dropped_frames;
    }

private:
    size_t m_current_frame_index{0};
    size_t m_dropped_frames{0};
    bool m_frame_open{false};

    Span<NodeInfo const> m_nodes;
    Vector<FrameTraceInfo, MAX_TRACED_FRAMES> m_frames_info;
};

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP