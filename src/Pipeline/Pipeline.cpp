#include "ducta/Pipeline/Pipeline.hpp"

#include <boost/range/adaptor/reversed.hpp>

namespace ducta {
namespace Pipeline {

Pipeline::Pipeline(Clock&& clock)
: m_clock(std::move(clock))
{
}

void Pipeline::configure(Nodes& nodes, std::vector<std::string> order)
{
    m_init_order.clear();
    m_nodes.clear();
    for (auto const& name : order)
    {
        m_init_order.emplace_back(name, std::ref(nodes.at(name)));
        m_nodes.emplace_back(name, std::ref(nodes.at(name)));
    }

    
}

void Pipeline::configure(Nodes& nodes, std::vector<std::string> init_order, std::vector<std::string> exec_order)
{
    m_init_order.clear();
    for (auto const& name : init_order)
    {
        m_init_order.emplace_back(name, std::ref(nodes.at(name)));
    }

    m_nodes.clear();
    for (auto const& name : exec_order)
    {
        m_nodes.emplace_back(name, std::ref(nodes.at(name)));
    }
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
    for (auto& node : m_nodes)
    {
        auto const& name = node.first;
        node.second.get().iterate();
    }

    return true;
}

void Pipeline::release()
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
    m_pipeline.release();
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