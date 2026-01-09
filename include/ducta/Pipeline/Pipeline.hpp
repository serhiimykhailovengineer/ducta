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

public:
    Pipeline(Clock&& clock);

    void addElement(std::string const& name, Node& element);

    void init();
    bool iterate();
    void stop();

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