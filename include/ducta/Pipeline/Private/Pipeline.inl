#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINE_INL
#define DUCTA_PIPELINE_PRIVATE_PIPELINE_INL

#include "ducta/Pipeline/Pipeline.hpp"

#include "ducta/Core/Algorithms/Adapters.hpp"
#include "ducta/Core/Config.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {

DUCTA_DECL Pipeline::Pipeline(Clock&& clock)
    : m_clock(::ducta::move(clock))
{
}

DUCTA_DECL bool Pipeline::configure(
    Nodes& nodes,
    Config const& config,
    Configuration::Diagnostics& diagnostics,
    TraceCallback trace_callback)
{
    using Configuration::PropNode;

    clear();
    if (!Configuration::validate(PropNode{}, config, diagnostics))
    {
        return false;
    }
    size_t const errors = diagnostics.error_count();

    for (auto const& name : config.order)
    {
        StringView const node_name{name.data(), name.size()};
        if (auto node_info = nodes.get_node(node_name))
        {
            m_init_order.emplace_back(*node_info);
            m_nodes.emplace_back(*node_info);
        }
        else
        {
            diagnostics.errorf(PropNode{}, "unknown node '{}' in pipeline order", node_name);
        }
    }

    Vector<size_t, DUCTA_MAX_CONNECTIONS> connected;
    for (size_t i = 0; i < config.connections.size(); ++i)
    {
        PortLink const& link = config.connections[i];
        StringView const output_node{link.output.node.data(), link.output.node.size()};
        StringView const output_name{link.output.name.data(), link.output.name.size()};
        StringView const input_node{link.input.node.data(), link.input.node.size()};
        StringView const input_name{link.input.name.data(), link.input.name.size()};

        auto source = nodes.get_node(output_node);
        auto target = nodes.get_node(input_node);
        if (!source || !target)
        {
            diagnostics.errorf(
                PropNode{}, "unknown node '{}' in connections", !source ? output_node : input_node);
            continue;
        }

        auto outputs = source->node.get_outputs();
        auto output = outputs.find(output_name);
        if (output == outputs.end())
        {
            diagnostics.errorf(PropNode{}, "node '{}' has no output '{}'", output_node, output_name);
            continue;
        }
        auto inputs = target->node.get_inputs();
        auto input = inputs.find(input_name);
        if (input == inputs.end())
        {
            diagnostics.errorf(PropNode{}, "node '{}' has no input '{}'", input_node, input_name);
            continue;
        }

        /* Only the connections actually made count: a rejected one reports its own problem */
        size_t fan_out = 0;
        bool connected_twice = false;
        for (size_t j : connected)
        {
            PortLink const& previous = config.connections[j];
            if (previous.output.node == link.output.node && previous.output.name == link.output.name)
            {
                ++fan_out;
            }
            if (previous.input.node == link.input.node && previous.input.name == link.input.name)
            {
                connected_twice = true;
            }
        }
        if (connected_twice)
        {
            diagnostics.errorf(
                PropNode{}, "input '{}.{}' is connected more than once", input_node, input_name);
            continue;
        }
        if (fan_out >= DUCTA_MAX_OUTPUT_CONNECTIONS)
        {
            diagnostics.errorf(
                PropNode{},
                "output '{}.{}' is connected to more than {} inputs",
                output_node,
                output_name,
                static_cast<unsigned long long>(DUCTA_MAX_OUTPUT_CONNECTIONS));
            continue;
        }

        IO::Connection connection = IO::bind(output->second, input->second);
        if (!connection)
        {
            diagnostics.errorf(
                PropNode{},
                "cannot connect '{}.{}' to '{}.{}': incompatible types",
                output_node,
                output_name,
                input_node,
                input_name);
            continue;
        }
        m_connections.push_back(::ducta::move(connection));
        connected.push_back(i);
    }

    if (diagnostics.error_count() != errors)
    {
        clear();
        return false;
    }

    m_trace_enabled = config.should_trace_execution;
    m_target_frame_duration = config.target_frame_duration;
    m_trace_flush_threshold = config.trace_flush_threshold;
    m_trace_callback = trace_callback;
    m_iteration_strategy =
        m_trace_enabled ? &Pipeline::iterateWithTrace : &Pipeline::iterateWithoutTrace;

    m_trace_info.configure(Span<NodeInfo const>(m_nodes.data(), m_nodes.size()));
    m_event_wiring.connect(m_nodes, &m_clock, m_trace_enabled ? &m_trace_info : nullptr);
    return true;
}

DUCTA_DECL bool Pipeline::configure(Nodes& nodes, Config const& config, TraceCallback trace_callback)
{
    Configuration::Diagnostics diagnostics;
    bool const ok = configure(nodes, config, diagnostics, trace_callback);
    for (auto const& diagnostic : diagnostics)
    {
        DUCTA_LOG_ERROR("{}", StringView{diagnostic.message.data(), diagnostic.message.size()});
    }
    return ok;
}

DUCTA_DECL void Pipeline::clear()
{
    m_event_wiring.disconnect();
    m_connections.clear();
    m_init_order.clear();
    m_nodes.clear();
    m_trace_info.configure(Span<NodeInfo const>(m_nodes.data(), m_nodes.size()));
    m_trace_enabled = false;
    m_trace_callback = {};
    m_iteration_strategy = &Pipeline::iterateWithoutTrace;
}

DUCTA_DECL void Pipeline::init()
{
    // release() disconnects the events: connect them again, so that every run delivers them
    m_event_wiring.connect(m_nodes, &m_clock, m_trace_enabled ? &m_trace_info : nullptr);

    size_t initialized = 0;
#if defined(__cpp_exceptions)
    try
    {
#endif
        for (auto& node_info : m_init_order)
        {
            node_info.node.init();
            ++initialized;
        }
#if defined(__cpp_exceptions)
    }
    catch (...)
    {
        // Release the nodes initialised so far, in reverse order, then pass the error on
        while (initialized > 0)
        {
            m_init_order[--initialized].node.release();
        }
        m_event_wiring.disconnect();
        throw;
    }
#endif
}

DUCTA_DECL bool Pipeline::iterate()
{
    return (this->*m_iteration_strategy)();
}

DUCTA_DECL bool Pipeline::iterateWithoutTrace()
{
    auto const frame_start_time = m_clock.now();

    for (auto& node_info : m_nodes)
    {
        auto const start_node = m_clock.now();
        try
        {
            auto result = node_info.node.iterate(start_node);

            if (result)
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
            DUCTA_LOG_ERROR(
                "Exception during iteration of node: {}, what(): {}", node_info.name, e.what());
            return false;
        }
    }

    auto const frame_duration = m_clock.now() - frame_start_time;
    if (m_target_frame_duration > frame_duration)
    {
        m_clock.sleepFor(m_target_frame_duration - frame_duration);
    }

    return true;
}

DUCTA_DECL bool Pipeline::iterateWithTrace()
{
    auto const frame_start_time = m_clock.now();
    m_trace_info.frame_start(frame_start_time);

    size_t node_index = 0;
    for (auto& node_info : m_nodes)
    {
        auto const start_node = m_clock.now();
        try
        {
            auto result = node_info.node.iterate(start_node);
            m_trace_info.node_duration(node_index++, m_clock.now() - start_node);

            if (result)
            {
                if (!*result)
                {
                    m_trace_info.frame_end(m_clock.now());
                    return false; // Stop iteration as requested by the node
                }
            }
            else
            {
                m_trace_info.frame_end(m_clock.now());
                DUCTA_LOG_ERROR("Error during iteration of node: {}", node_info.name);
                return false;
            }
        }
        catch (const std::exception& e)
        {
            m_trace_info.frame_end(m_clock.now());
            DUCTA_LOG_ERROR(
                "Exception during iteration of node: {}, what(): {}", node_info.name, e.what());
            return false;
        }
    }

    m_trace_info.frame_end(m_clock.now());

    if (m_trace_callback && m_trace_info.pending_frames() >= m_trace_flush_threshold)
    {
        m_trace_info.flush(m_trace_callback);
    }

    auto const frame_duration = m_clock.now() - frame_start_time;
    if (m_target_frame_duration > frame_duration)
    {
        m_clock.sleepFor(m_target_frame_duration - frame_duration);
    }

    return true;
}

DUCTA_DECL void Pipeline::release()
{
    for (auto& node_info : ducta::Adapters::Reversed(m_init_order))
    {
        node_info.node.release();
    }
    m_event_wiring.disconnect();
}

DUCTA_DECL PipelineEngine::PipelineEngine(Pipeline& pipeline, StopToken&& stop_token)
    : m_pipeline{pipeline}
    , m_stop_token{::ducta::move(stop_token)}
{
    m_pipeline.init();
}

DUCTA_DECL PipelineEngine::~PipelineEngine()
{
    m_pipeline.release();
}

DUCTA_DECL int PipelineEngine::run()
{
    while (true)
    {
        if (m_stop_token.stopRequested())
        {
            DUCTA_LOG_INFO("Stop requested, exiting pipeline engine.");
            break;
        }

        if (!m_pipeline.iterate())
        {
            DUCTA_LOG_INFO("Pipeline iteration requested stop, exiting pipeline engine.");
            break;
        }
    }

    return EXIT_SUCCESS;
}

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINE_INL