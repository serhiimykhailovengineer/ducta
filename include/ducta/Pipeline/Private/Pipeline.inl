#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINE_INL
#define DUCTA_PIPELINE_PRIVATE_PIPELINE_INL

#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/Core/Algorithms/Adapters.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {

Pipeline::Pipeline(Clock&& clock)
: m_clock(::ducta::move(clock))
{
}

bool Pipeline::configure(Nodes& nodes, Config const& config)
{
    m_trace_enabled = config.should_trace_execution;

    m_init_order.clear();
    m_nodes.clear();

    for (auto const& name : config.order)
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

    m_trace_info.configure(Span<StringView const>(config.order.data(), config.order.size()));
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
    if(m_trace_enabled)
    {
        m_trace_info.frame_start(m_clock.now());
    }

    for (auto& node_info : m_nodes)
    {
        auto const start_node = m_clock.now();
        try 
        {
            auto result = node_info.node.iterate(start_node);
            if(m_trace_enabled)
            {
                m_trace_info.node_duration(m_clock.now() - start_node);
            }

            if(result)
            {
                if (!*result)
                {
                    if(m_trace_enabled)
                    {
                        m_trace_info.frame_end(m_clock.now());
                    }
                    return false; // Stop iteration as requested by the node
                }
            }
            else
            {
                if(m_trace_enabled)
                {
                    m_trace_info.frame_end(m_clock.now());
                }
                DUCTA_LOG_ERROR("Error during iteration of node: {}", node_info.name);
                return false;
            }
        } 
        catch (const std::exception& e) 
        {
            if(m_trace_enabled)
            {
                m_trace_info.frame_end(m_clock.now());
            }
            DUCTA_LOG_ERROR("Exception during iteration of node: {}, what(): {}", node_info.name, e.what());
            return false;
        }
    }
    if(m_trace_enabled)
    {
        m_trace_info.frame_end(m_clock.now());
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

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINE_INL