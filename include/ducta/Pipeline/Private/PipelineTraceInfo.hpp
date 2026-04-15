#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP
#define DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP

#include "ducta/Chrono/Chrono.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/Span.hpp"
#include "ducta/Core/Assert.hpp"

namespace ducta {
namespace Pipeline {

struct FrameTraceInfo
{ 
    size_t frame_index;
    Chrono::TimestampUS frame_start; 
    Chrono::TimestampUS frame_end; 
    Vector<Chrono::TimestampUS> nodes_durations; 
};

namespace Private {

class PipelineTraceInfo
{
public:

    void configure(Span<StringView const> const& node_names)
    {
        m_node_names = node_names;
        m_frames_info.clear();
        m_current_frame_index = 0;
    }

    void frame_start(Chrono::TimestampUS ts)
    {
        DUCTA_ASSERT(!m_frame_open);
    
        m_frames_info.emplace_back();

        auto& current_frame_info = m_frames_info.back();
        current_frame_info.frame_index = m_current_frame_index++;
        current_frame_info.nodes_durations.resize(m_node_names.size(), Chrono::TimestampUS{0});
        current_frame_info.frame_start = ts;

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

        DUCTA_ASSERT(node_index < m_node_names.size());
        current_frame_info.nodes_durations[node_index] = ts;
    }

    template <typename Callback>
    void flush(Callback&& callback)
    {
        if (m_frames_info.empty())
        {
            return;
        }

        callback(m_node_names, Span<FrameTraceInfo const>(m_frames_info.data(), m_frames_info.size()));
        m_frames_info.clear();
    }

    size_t pending_frames() const
    {
        return m_frames_info.size();
    }

private:
    size_t m_current_frame_index{0};
    bool m_frame_open{false};

    Span<StringView const> m_node_names;
    Vector<FrameTraceInfo> m_frames_info;
};

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP