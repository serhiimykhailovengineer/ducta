#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/DefineConfig.hpp"
#include "ducta/Configuration/Duration.hpp"

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Pipeline/Nodes.hpp"
#include "ducta/Pipeline/StopToken.hpp"

#include "ducta/Pipeline/Private/EventWiring.hpp"
#include "ducta/Pipeline/Private/PipelineTraceInfo.hpp"

#include "ducta/Core/Types/FunctionRef.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/Span.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"

namespace ducta {
namespace Pipeline {

/*
 * @brief How the nodes are run; decoded from e.g.
 *   { "order": ["Adder", "Gain"], "target_frame_duration": "1ms" }
 */
struct PipelineConfig
{
    /* Names of the nodes in execution order (also the order of init(); release() is reversed) */
    Vector<NodeName, DUCTA_MAX_NODES> order;
    /* Minimal duration of one iteration; 0 runs as fast as possible */
    Chrono::TimestampUS target_frame_duration{0};
    bool should_trace_execution{false};
    /* Traced frames collected before they are passed to the trace callback */
    size_t trace_flush_threshold{100};
};

class Pipeline
{
public:
    using Config = PipelineConfig;

    using TraceCallback =
        FunctionRef<void(Span<StringView const> node_names, Span<FrameTraceInfo const> frames_info)>;

public:
    Pipeline(Clock&& clock);
    template <
        typename ClockType,
        typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<ClockType>, Clock>>>
    Pipeline(ClockType& clock)
        : Pipeline(Clock(clock))
    {
    }

    bool configure(Nodes& nodes, Config const& config, TraceCallback trace_callback = {});

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

    Vector<NodeInfo, DUCTA_MAX_NODES> m_init_order;
    Vector<NodeInfo, DUCTA_MAX_NODES> m_nodes;
    Private::EventWiring m_event_wiring;
};

class PipelineEngine
{
public:
    PipelineEngine(Pipeline& pipeline, StopToken&& stop_token);

    template <
        typename StopTokenType,
        typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<StopTokenType>, StopToken>>>
    PipelineEngine(Pipeline& pipeline, StopTokenType& stop_token)
        : PipelineEngine(pipeline, StopToken(stop_token))
    {
    }

    ~PipelineEngine();

    int run();

private:
    Pipeline& m_pipeline;
    StopToken m_stop_token;
};

} // namespace Pipeline
} // namespace ducta

DEFINE_CONFIG(
    ducta::Pipeline::PipelineConfig,
    ("order", order),
    ("target_frame_duration", target_frame_duration),
    ("should_trace_execution", should_trace_execution),
    ("trace_flush_threshold", trace_flush_threshold));

namespace ducta {
namespace Pipeline {
namespace Private {

template <class Names>
bool has_duplicates(Names const& names)
{
    for (size_t i = 0; i < names.size(); ++i)
    {
        for (size_t j = i + 1; j < names.size(); ++j)
        {
            if (names[i] == names[j])
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace Private
} // namespace Pipeline
} // namespace ducta

DEFINE_CONFIG_RULES(
    ducta::Pipeline::PipelineConfig,
    ("order", !config.order.empty(), "must not be empty"),
    ("order",
     !::ducta::Pipeline::Private::has_duplicates(config.order),
     "must not contain a node twice"),
    ("target_frame_duration", config.target_frame_duration.count() >= 0, "must not be negative"),
    ("trace_flush_threshold", config.trace_flush_threshold > 0, "must be positive"));

#if !defined(DUCTA_SEPARATE_COMPILATION)
#include "ducta/Pipeline/Private/Pipeline.inl"
#endif

#endif // DUCTA_PIPELINE_PIPELINE_HPP