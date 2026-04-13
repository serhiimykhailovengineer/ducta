#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Pipeline/Nodes.hpp"

#include "ducta/Pipeline/Private/PipelineTraceInfo.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Types/Span.hpp"

namespace ducta {
namespace Pipeline {

struct PipelineConfig
{
    Vector<StringView, 50> order;
    bool should_trace_execution{false};
};

class Pipeline
{
public:
    using Config = PipelineConfig;

public:
    Pipeline(Clock&& clock);

    bool configure(Nodes& nodes, Config const& config);

    void init();
    bool iterate();
    void release();

private:
    Clock m_clock;
    bool m_trace_enabled{false};
    Private::PipelineTraceInfo m_trace_info;

    Vector<NodeInfo, 50> m_init_order;
    Vector<NodeInfo, 50> m_nodes;
};

class PipelineEngine
{
public:
    PipelineEngine(Pipeline& pipeline);
    ~PipelineEngine();

    int run();

private:
    Pipeline& m_pipeline;
};

} // namespace Pipeline
} // namespace ducta

#include "ducta/Pipeline/Private/Pipeline.inl"

#endif // DUCTA_PIPELINE_PIPELINE_HPP