#ifndef DUCTA_PIPELINE_PRIVATE_PIPELINE_INL
#define DUCTA_PIPELINE_PRIVATE_PIPELINE_INL

#include "ducta/Pipeline/Pipeline.hpp"

#if defined(__cpp_exceptions)
#include <exception>
#endif

#include "ducta/Core/Algorithms/Adapters.hpp"
#include "ducta/Core/Config.hpp"
#include "ducta/Utils/Log.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

inline StringView error_message(Error const& error)
{
    return StringView{error.message.data(), error.message.size()};
}

/*
 * Called in a catch block: a node broke its contract by throwing from call (init, iterate,
 * release); logs the node and what() if available
 */
inline void log_node_exception(char const* call, StringView node_name)
{
#if defined(__cpp_exceptions)
    try
    {
        throw;
    }
    catch (std::exception const& e)
    {
        DUCTA_LOG_ERROR("{}() of node '{}' failed: exception: {}", call, node_name, e.what());
    }
    catch (...)
    {
        DUCTA_LOG_ERROR("{}() of node '{}' failed: unknown exception", call, node_name);
    }
#else
    (void)call;
    (void)node_name;
#endif
}

inline unsigned long long to_ull(Chrono::TimestampUS duration)
{
    return static_cast<unsigned long long>(duration.count() > 0 ? duration.count() : 0);
}

} // namespace Private

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
    DUCTA_LOG_DEBUG("Pipeline configuration started");

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
        bool connected_twice = false;
        for (size_t j : connected)
        {
            PortLink const& previous = config.connections[j];
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
        /* Asks the output itself, so inputs bound to it outside the configuration count too */
        if (output->second.full())
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
        DUCTA_LOG_DEBUG("Connected '{}.{}' -> '{}.{}'", output_node, output_name, input_node, input_name);
    }

    if (diagnostics.error_count() != errors)
    {
        clear();
        return false;
    }

    m_target_frame_duration = config.target_frame_duration;
#if DUCTA_ENABLE_TRACING
    m_trace_enabled = config.should_trace_execution;
    m_trace_flush_threshold = config.trace_flush_threshold;
    m_trace_callback = trace_callback;
    m_trace_info.configure(Span<NodeInfo const>(m_nodes.data(), m_nodes.size()));
#else
    (void)trace_callback;
    if (config.should_trace_execution)
    {
        diagnostics.warning(
            PropNode{},
            "should_trace_execution is ignored: tracing is disabled at compile time "
            "(DUCTA_ENABLE_TRACING=0)");
    }
#endif
    if (!m_event_wiring.connect(m_nodes, &m_clock, traceInfo(), &diagnostics))
    {
        clear();
        return false;
    }

    DUCTA_LOG_INFO(
        "Pipeline configured: {} nodes, {} connections, target frame duration {} us",
        static_cast<unsigned long long>(m_nodes.size()),
        static_cast<unsigned long long>(m_connections.size()),
        Private::to_ull(m_target_frame_duration));
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
    m_trace_enabled = false;
#if DUCTA_ENABLE_TRACING
    m_trace_info.configure(Span<NodeInfo const>(m_nodes.data(), m_nodes.size()));
    m_trace_callback = {};
#endif
}

DUCTA_DECL bool Pipeline::init()
{
    // release() disconnects the events: connect them again, so that every run delivers them.
    // The same nodes were connected by configure(), so this fits the capacities too.
    m_event_wiring.connect(m_nodes, &m_clock, traceInfo());

    m_failed = false;
    m_frames = 0;
    m_overrun_frames = 0;
    m_longest_frame = Chrono::TimestampUS{0};

    // From the first node not initialised yet (all of them after release())
    for (size_t i = m_initialized; i < m_init_order.size(); ++i)
    {
        auto& node_info = m_init_order[i];
        DUCTA_LOG_DEBUG("Initialising node '{}'", node_info.name);
#if defined(__cpp_exceptions)
        Expected<bool, Error> result{true};
        try
        {
            result = node_info.node.init();
        }
        catch (...)
        {
            Private::log_node_exception("init", node_info.name);
            m_failed = true;
            return false; // not counted as initialised: not released
        }
#else
        auto const result = node_info.node.init();
#endif
        if (!result)
        {
            DUCTA_LOG_ERROR(
                "init() of node '{}' failed: {}", node_info.name, Private::error_message(result.error()));
            m_failed = true;
            return false;
        }
        if (!*result)
        {
            DUCTA_LOG_ERROR("init() of node '{}' returned false", node_info.name);
            m_failed = true;
            return false;
        }
        ++m_initialized; // counted only now: a node whose init() failed is not released
    }
    DUCTA_LOG_INFO("Pipeline initialised: {} nodes", static_cast<unsigned long long>(m_initialized));
    return true;
}

DUCTA_DECL bool Pipeline::iterate()
{
#if DUCTA_ENABLE_TRACING
    if (m_trace_enabled)
    {
        return iterateWithTrace();
    }
#endif
    return iterateWithoutTrace();
}

DUCTA_DECL bool Pipeline::iterateNode(NodeInfo& node_info, Chrono::TimestampUS frame_start_time)
{
#if defined(__cpp_exceptions)
    Expected<bool, Error> result{true};
    try
    {
        result = node_info.node.iterate(frame_start_time);
    }
    catch (...)
    {
        Private::log_node_exception("iterate", node_info.name);
        m_failed = true;
        return false;
    }
#else
    auto const result = node_info.node.iterate(frame_start_time);
#endif
    if (!result)
    {
        DUCTA_LOG_ERROR(
            "iterate() of node '{}' failed: {}", node_info.name, Private::error_message(result.error()));
        m_failed = true;
        return false;
    }
    if (!*result)
    {
        DUCTA_LOG_INFO("Node '{}' requested to stop the pipeline", node_info.name);
        return false;
    }
    return true;
}

DUCTA_DECL void Pipeline::sleepUntilFrameEnd(Chrono::TimestampUS frame_start_time)
{
    if (m_target_frame_duration.count() <= 0)
    {
        return; // as fast as possible: no need to read the clock again
    }
    auto const frame_duration = m_clock.now() - frame_start_time;
    ++m_frames;
    if (frame_duration > m_longest_frame)
    {
        m_longest_frame = frame_duration;
    }
    if (m_target_frame_duration > frame_duration)
    {
        m_clock.sleepFor(m_target_frame_duration - frame_duration);
    }
    else if (frame_duration > m_target_frame_duration && ++m_overrun_frames == 1)
    {
        // Once per run: the count is in the summary of release()
        DUCTA_LOG_WARNING(
            "Frame took {} us, longer than the target frame duration {} us",
            Private::to_ull(frame_duration),
            Private::to_ull(m_target_frame_duration));
    }
}

DUCTA_DECL bool Pipeline::iterateWithoutTrace()
{
    auto const frame_start_time = m_clock.now();

    for (auto& node_info : m_nodes)
    {
        if (!iterateNode(node_info, frame_start_time))
        {
            return false;
        }
    }

    sleepUntilFrameEnd(frame_start_time);
    return true;
}

#if DUCTA_ENABLE_TRACING
DUCTA_DECL bool Pipeline::iterateWithTrace()
{
    auto const frame_start_time = m_clock.now();
    m_trace_info.frame_start(frame_start_time);

    /* Closes the frame however it ends (including an exception passed on), or the next
     * frame_start() fails */
    struct FrameCloser
    {
        Private::PipelineTraceInfo& trace;
        Clock& clock;

        ~FrameCloser()
        {
            if (trace.frame_open())
            {
                trace.frame_end(clock.now());
            }
        }
    } const closer{m_trace_info, m_clock};

    /* The end of a node is the start of the next one: one clock read per node */
    auto node_start_time = frame_start_time;
    size_t node_index = 0;
    for (auto& node_info : m_nodes)
    {
        bool const keep_running = iterateNode(node_info, frame_start_time);
        auto const node_end_time = m_clock.now();
        m_trace_info.node_duration(node_index++, node_end_time - node_start_time);
        node_start_time = node_end_time;
        if (!keep_running)
        {
            m_trace_info.frame_end(node_end_time);
            return false;
        }
    }
    m_trace_info.frame_end(node_start_time);

    if (m_trace_callback && m_trace_info.pending_frames() >= m_trace_flush_threshold)
    {
        m_trace_info.flush(m_trace_callback);
    }

    sleepUntilFrameEnd(frame_start_time);
    return true;
}
#endif

DUCTA_DECL void Pipeline::release()
{
    size_t const initialized = m_initialized;
    while (m_initialized > 0)
    {
        auto& node_info = m_init_order[m_initialized - 1];
        DUCTA_LOG_DEBUG("Releasing node '{}'", node_info.name);
#if defined(__cpp_exceptions)
        try
        {
#endif
            node_info.node.release();
#if defined(__cpp_exceptions)
        }
        catch (...)
        {
            // It broke the contract: nothing more can be done for it, the others are released
            Private::log_node_exception("release", node_info.name);
            m_failed = true;
        }
#endif
        --m_initialized;
    }
    m_event_wiring.disconnect();
    if (initialized != 0)
    {
        logRunSummary(initialized);
    }
}

DUCTA_DECL void Pipeline::logRunSummary(size_t released)
{
    DUCTA_LOG_INFO("Pipeline released: {} nodes", static_cast<unsigned long long>(released));
    if (m_overrun_frames != 0)
    {
        DUCTA_LOG_WARNING(
            "{} of {} frames took longer than the target frame duration {} us (longest: {} us)",
            static_cast<unsigned long long>(m_overrun_frames),
            static_cast<unsigned long long>(m_frames),
            Private::to_ull(m_target_frame_duration),
            Private::to_ull(m_longest_frame));
    }
}

DUCTA_DECL PipelineEngine::PipelineEngine(Pipeline& pipeline, StopToken&& stop_token)
    : m_pipeline{pipeline}
    , m_stop_token{::ducta::move(stop_token)}
{
#if defined(__cpp_exceptions)
    try
    {
#endif
        m_initialized = m_pipeline.init(); // a failure is logged by Pipeline::init
#if defined(__cpp_exceptions)
    }
    catch (...)
    {
        // The destructor does not run: release the nodes initialised before the exception
        m_pipeline.release();
        throw;
    }
#endif
}

DUCTA_DECL PipelineEngine::~PipelineEngine()
{
    // Also after a failed init(): the nodes initialised before the failing one
#if defined(__cpp_exceptions)
    try
    {
#endif
        m_pipeline.release();
#if defined(__cpp_exceptions)
    }
    catch (std::exception& e)
    {
        DUCTA_LOG_ERROR("Exception in destructor: {}", e.what());
    }
    catch (...)
    {
        DUCTA_LOG_ERROR("Exception in destructor: unknown");
    }
#endif
}

DUCTA_DECL int PipelineEngine::run()
{
    if (!m_initialized)
    {
        DUCTA_LOG_ERROR("Pipeline not initialized: cannot run");
        return EXIT_FAILURE;
    }

    DUCTA_LOG_INFO("Pipeline running");
#if defined(__cpp_exceptions)
    try
    {
#endif
        while (true)
        {
            if (m_stop_token.stopRequested())
            {
                DUCTA_LOG_INFO("Stop requested: stopping the pipeline");
                break;
            }

            if (!m_pipeline.iterate())
            {
                break; // the reason is logged by Pipeline::iterate
            }
        }
#if defined(__cpp_exceptions)
    }
    catch (std::exception& e)
        {
            DUCTA_LOG_ERROR("Exception in run(): {}", e.what());
            return EXIT_FAILURE;
        }
        catch (...)
        {
            DUCTA_LOG_ERROR("Exception in run(): unknown");
            return EXIT_FAILURE;
        }
#endif
    return m_pipeline.failed() ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_PIPELINE_INL