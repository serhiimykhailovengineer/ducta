#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP
#define DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP

#include "ducta/Chrono/Chrono.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/Span.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

struct FrameTraceInfo { 
    Chrono::TimestampUS frame_start; 
    Chrono::TimestampUS frame_end; 
    Vector<Chrono::TimestampUS> nodes_durations; 
};

class PipelineTraceInfo
{
public:

    void configure(Span<StringView const> const& node_names)
    {
        m_node_names = node_names;
        m_frames_info.clear();
    }

    void frame_start(Chrono::TimestampUS ts)
    {
        m_frames_info.emplace_back();

        auto& current_frame_info = m_frames_info.back();
        current_frame_info.nodes_durations.reserve(m_node_names.size());
        current_frame_info.frame_start = ts;
    }

    void frame_end(Chrono::TimestampUS ts)
    {
        auto& current_frame_info = m_frames_info.back();
        current_frame_info.frame_end = ts;
    }

    void node_duration(Chrono::TimestampUS ts)
    {
        auto& current_frame_info = m_frames_info.back();
        current_frame_info.nodes_durations.emplace_back(ts);
    }

private:
    Span<StringView const> m_node_names;
    Vector<FrameTraceInfo> m_frames_info;
};

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINETRACEINFO_HPP