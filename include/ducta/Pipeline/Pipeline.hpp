#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeInfo.hpp"
#include "ducta/Pipeline/Nodes.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Types/Span.hpp"

namespace ducta {
namespace Pipeline {

class Pipeline
{
public:
    Pipeline(Clock&& clock);

    bool configure(Nodes& nodes, Span<StringView> const& order);
    bool configure(Nodes& nodes, Span<StringView> const& init_order, Span<StringView> const& exec_order);

    void init();
    bool iterate();
    void release();

private:
    Clock m_clock;

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

#endif // DUCTA_PIPELINE_PIPELINE_HPP