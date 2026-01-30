#include "ducta/Pipeline/Pipeline.hpp"

#include <boost/range/adaptor/reversed.hpp>

#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {

Pipeline::Pipeline(Clock&& clock)
: m_clock(std::move(clock))
{
}

bool Pipeline::configure(Nodes& nodes, std::vector<std::string> order)
{
    m_init_order.clear();
    m_nodes.clear();
    for (auto const& name : order)
    {
        auto node_it = nodes.find(name);
        if (node_it == nodes.end())
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
        m_init_order.emplace_back(name, std::ref(node_it->second));
        m_nodes.emplace_back(name, std::ref(node_it->second));
    }
    return true;
}

bool Pipeline::configure(Nodes& nodes, std::vector<std::string> init_order, std::vector<std::string> exec_order)
{
    m_init_order.clear();
    for (auto const& name : init_order)
    {
        auto node_it = nodes.find(name);
        if (node_it == nodes.end())
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
        m_init_order.emplace_back(name, std::ref(node_it->second));
    }

    m_nodes.clear();
    for (auto const& name : exec_order)
    {
        auto node_it = nodes.find(name);
        if (node_it == nodes.end())
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
        m_nodes.emplace_back(name, std::ref(node_it->second));
    }
    return true;
}

void Pipeline::init()
{
    for (auto& node : m_init_order)
    {
        node.second.get().init();
    }
}

bool Pipeline::iterate()
{
    for (auto& node : m_nodes)
    {
        try 
        {
            auto result = node.second.get().iterate();
            if(result)
            {
                if (!*result)
                {
                    return false; // Stop iteration as requested by the node
                }
            }
            else
            {
                DUCTA_LOG_ERROR("Error during iteration of node: {}", node.first);
                return false;
            }
        } 
        catch (const std::exception& e) 
        {
            DUCTA_LOG_ERROR("Exception during iteration of node: {}, what(): {}", node.first, e.what());
            return false;
        }
    }

    return true;
}

void Pipeline::release()
{
    for (auto& node : boost::adaptors::reverse(m_init_order))
    {
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