#include "ducta/Pipeline/Pipeline.hpp"

#include <boost/range/adaptor/reversed.hpp>

namespace ducta {
namespace Pipeline {

Pipeline::Pipeline(Clock&& clock)
: m_clock(std::move(clock))
{

}

void Pipeline::addElement(std::string const& name, Node& element)
{
    m_nodes.emplace_back(name, std::ref(element));
}

void Pipeline::init()
{
    for (auto& node : m_init_order)
    {
        auto const& name = node.first;
        node.second.get().init();
    }
}

bool Pipeline::iterate()
{
    auto frame_start = m_clock.now();
    for (auto& node : m_nodes)
    {
        auto const& name = node.first;
        auto element_start = m_clock.now();
        node.second.get().iterate();
    }
    auto frame_end = m_clock.now();

    return true;
}

void Pipeline::stop()
{
    for (auto& node : boost::adaptors::reverse(m_init_order))
    {
        auto const& name = node.first;
        node.second.get().release();
    }
}

PipelineEngine::PipelineEngine(Pipeline& pipeline)
: m_pipeline{pipeline}
{
    m_pipeline.init();
}

PipelineEngine::~PipelineEngine()
{
    m_pipeline.stop();
}

int PipelineEngine::run()
{
    while (true)
    {
        if (!m_pipeline.iterate())
        {
            break;
        }
    }

    return EXIT_SUCCESS;
}

} // namespace Pipeline
} // namespace ducta