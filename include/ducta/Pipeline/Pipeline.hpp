#ifndef DUCTA_PIPELINE_PIPELINE_HPP
#define DUCTA_PIPELINE_PIPELINE_HPP

#include <vector>

#include "ducta/Pipeline/Node.hpp"

namespace ducta {
namespace Pipeline {

class Pipeline
{
private:
// public:
//     PipelineEngine();
//     ~PipelineEngine();

//     template <typename T>
//     void addElement(T& element);

    void init();
    void iterate();
    void stop();

private:
    std::vector<std::reference_wrapper<Node>> m_nodes;
};

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PIPELINE_HPP