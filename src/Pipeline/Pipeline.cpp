#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/Core/Algorithms/Adapters.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {

Pipeline::Pipeline(Clock&& clock)
: m_clock(::ducta::move(clock))
{
}

bool Pipeline::configure(Nodes& nodes, Span<StringView> const& order)
{
    m_init_order.clear();
    m_nodes.clear();
    for (auto const& name : order)
    {
        if (auto node_info = nodes.get_node(name))
        {
            m_init_order.emplace_back(*node_info);
            m_nodes.emplace_back(*node_info);
        }
        else
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
    }
    return true;
}

bool Pipeline::configure(Nodes& nodes, Span<StringView> const& init_order, Span<StringView> const& exec_order)
{
    m_init_order.clear();
    for (auto const& name : init_order)
    {
        if (auto node_info = nodes.get_node(name))
        {
            m_init_order.emplace_back(*node_info);
        }
        else
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
    }

    m_nodes.clear();
    for (auto const& name : exec_order)
    {
        if (auto node_info = nodes.get_node(name))
        {
            m_nodes.emplace_back(*node_info);
        }
        else
        {
            DUCTA_LOG_ERROR("Node not found: {}", name);
            m_init_order.clear();
            m_nodes.clear();
            return false;
        }
    }
    return true;
}

void Pipeline::init()
{
    for (auto& node_info : m_init_order)
    {
        node_info.node.init();
    }
}

bool Pipeline::iterate()
{
    for (auto& node_info : m_nodes)
    {
        try 
        {
            auto result = node_info.node.iterate();
            if(result)
            {
                if (!*result)
                {
                    return false; // Stop iteration as requested by the node
                }
            }
            else
            {
                DUCTA_LOG_ERROR("Error during iteration of node: {}", node_info.name);
                return false;
            }
        } 
        catch (const std::exception& e) 
        {
            DUCTA_LOG_ERROR("Exception during iteration of node: {}, what(): {}", node_info.name, e.what());
            return false;
        }
    }

    return true;
}

void Pipeline::release()
{
    for (auto& node_info : ducta::Adapters::Reversed(m_init_order))
    {
        node_info.node.release();
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