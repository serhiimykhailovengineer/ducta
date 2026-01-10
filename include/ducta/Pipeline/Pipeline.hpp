#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include <vector>

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/Node.hpp"

namespace ducta {
namespace Pipeline {

class Pipeline
{
public:
    using Nodes = std::map<std::string, Node>;

public:
    Pipeline(Clock&& clock);

    void configure(Nodes& nodes, std::vector<std::string> order);
    void configure(Nodes& nodes, std::vector<std::string> init_order, std::vector<std::string> exec_order);

    void init();
    bool iterate();
    void release();

private:
    Clock m_clock;
    std::vector<std::pair<std::string, std::reference_wrapper<Node>>> m_init_order;
    std::vector<std::pair<std::string, std::reference_wrapper<Node>>> m_nodes;
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