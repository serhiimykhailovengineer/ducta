#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Pipeline/Nodes.hpp"
#include "ducta/Pipeline/StopToken.hpp"

#include "ducta/Pipeline/Private/PipelineTraceInfo.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Types/Span.hpp"
#include "ducta/Core/Types/FunctionRef.hpp"

namespace ducta {
namespace Pipeline {

struct PipelineConfig
{
    Vector<StringView, 50> order;
    Chrono::TimestampUS target_frame_duration{0};
    bool should_trace_execution{false};
    size_t trace_flush_threshold{100};
};

class Pipeline
{
public:
    using Config = PipelineConfig;

    using TraceCallback = FunctionRef<void(Span<StringView const> node_names, Span<FrameTraceInfo const> frames_info)>; 

public:
    Pipeline(Clock&& clock);
    template <typename ClockType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<ClockType>, Clock>>>
    Pipeline(ClockType& clock)
    : Pipeline(Clock(clock))
    {}

    bool configure(Nodes& nodes, Config const& config, TraceCallback trace_callback = nullptr);

    void init();
    bool iterate();
    void release();

private:
    using IterationStrategy = bool (Pipeline::*)();

    bool iterateWithTrace();
    bool iterateWithoutTrace();

    IterationStrategy m_iteration_strategy{&Pipeline::iterateWithoutTrace};

private:
    Clock m_clock;
    Chrono::TimestampUS m_target_frame_duration{0};
    bool m_trace_enabled{false};
    Private::PipelineTraceInfo m_trace_info;
    size_t m_trace_flush_threshold{100};
    TraceCallback m_trace_callback;

    Vector<NodeInfo, 50> m_init_order;
    Vector<NodeInfo, 50> m_nodes;
};

class PipelineEngine
{
public:
    PipelineEngine(Pipeline& pipeline, StopToken&& stop_token);

    template <typename StopTokenType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<StopTokenType>, StopToken>>>
    PipelineEngine(Pipeline& pipeline, StopTokenType& stop_token)
    : PipelineEngine(pipeline, StopToken(stop_token))
    {}

    ~PipelineEngine();

    int run();

private:
    Pipeline& m_pipeline;
    StopToken m_stop_token;
};

} // namespace Pipeline
} // namespace ducta

#include "ducta/Pipeline/Private/Pipeline.inl"

#endif // DUCTA_PIPELINE_PIPELINE_HPP