/**
 * @file DuctaEngine.hpp
 * @brief Creates the nodes of a pipeline from one configuration and runs the pipeline
 */

#ifndef DUCTA_DUCTA_ENGINE_HPP
#define DUCTA_DUCTA_ENGINE_HPP

#include <cstdlib>

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/DefineConfig.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"
#include "ducta/Core/Utility.hpp"

#include "ducta/Pipeline/Clock.hpp"
#include "ducta/Pipeline/NodeFactory.hpp"
#include "ducta/Pipeline/NodeManager.hpp"
#include "ducta/Pipeline/Pipeline.hpp"
#include "ducta/Pipeline/StopToken.hpp"

#include "ducta/Utils/Log.hpp"

namespace ducta {

/*
 * @brief Nodes to create (see NodeConfigs) and how to run them:
 *   {
 *     "nodes":    { "amp": { "type": "Gain", "config": { "gain": 2.0 } }, "sum": { "type": "Adder"
 * } }, "pipeline": { "order": ["sum", "amp"], "target_frame_duration": "1ms" }
 *   }
 */
struct DuctaEngineConfig
{
    Pipeline::NodeConfigs nodes;
    Pipeline::PipelineConfig pipeline;
};

/*
 * @brief Owns the nodes (NodeManager) and the Pipeline running them
 * @tparam Storage Storage policy of the nodes (see NodeStorage.hpp)
 *
 *     DuctaEngine<> engine{clock};
 *     if (!engine.configure(tree, diagnostics)) { ... report diagnostics ... }
 *     return engine.run(stop_token);
 */
template <class Storage = Pipeline::DefaultNodeStorage>
class DuctaEngine
{
public:
    using Config = DuctaEngineConfig;
    using TraceCallback = Pipeline::Pipeline::TraceCallback;

    template <
        typename ClockType,
        typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<ClockType>, Pipeline::Clock>>>
    explicit DuctaEngine(
        ClockType& clock,
        Pipeline::NodeFactory const& factory = Pipeline::NodeFactory::instance())
        : DuctaEngine(Pipeline::Clock{clock}, factory)
    {
    }

    explicit DuctaEngine(
        Pipeline::Clock clock,
        Pipeline::NodeFactory const& factory = Pipeline::NodeFactory::instance())
        : m_node_manager{factory}
        , m_pipeline{::ducta::move(clock)}
    {
    }

    /* The engine keeps references to clock and factory: a temporary factory would be destroyed
     * before configure() uses it */
    template <
        typename ClockType,
        typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<ClockType>, Pipeline::Clock>>>
    DuctaEngine(ClockType& clock, Pipeline::NodeFactory&& factory) = delete;
    DuctaEngine(Pipeline::Clock clock, Pipeline::NodeFactory&& factory) = delete;

    DuctaEngine(DuctaEngine const&) = delete;
    DuctaEngine& operator=(DuctaEngine const&) = delete;

    ~DuctaEngine()
    {
        clear();
    }

    /*
     * @brief Decode Config from the root of tree, then configure (see below)
     * @note The tree may be discarded afterwards
     */
    bool configure(
        Configuration::PropTree const& tree,
        Configuration::Diagnostics& diagnostics,
        TraceCallback trace_callback = {})
    {
        Config config;
        if (!Configuration::decode(tree.root(), config, diagnostics))
        {
            clear();
            return false;
        }
        return configure(config, diagnostics, trace_callback);
    }

    /*
     * @brief Replace the nodes by the ones of config and prepare the pipeline to run them
     *
     * Every problem (PipelineConfig rules, node creation, a node in the order that was not
     * created) is reported to diagnostics; created nodes missing from the order are reported
     * as warnings. All or nothing: on any error no node is kept.
     *
     * @return true if the engine is ready to run
     */
    bool configure(
        Config const& config,
        Configuration::Diagnostics& diagnostics,
        TraceCallback trace_callback = {})
    {
        if (m_running)
        {
            diagnostics.error(
                Configuration::PropNode{}, "DuctaEngine cannot be configured while running");
            return false;
        }
        clear();

        // config.pipeline is validated by Pipeline::configure
        if (!m_node_manager.configure(Pipeline::NodeManagerConfig{config.nodes}, diagnostics))
        {
            return false;
        }

        if (!m_pipeline.configure(m_node_manager.nodes(), config.pipeline, diagnostics, trace_callback))
        {
            clear();
            return false;
        }
        // Only for a pipeline that will run: nodes it never runs are probably a mistake
        for (auto const& entry : config.nodes)
        {
            StringView const name{entry.first.data(), entry.first.size()};
            if (!contains(config.pipeline.order, name))
            {
                diagnostics.warningf(
                    entry.second.config, "node '{}' is not in pipeline order and never runs", name);
            }
        }

        m_configured = true;
        return true;
    }

    /*
     * @brief Run the pipeline until stop is requested or a node stops it
     *
     * Initialises the nodes in pipeline order, iterates them, then releases them in reverse
     * order. The nodes are kept: the engine can run again or be reconfigured afterwards.
     * An exception of a node's init(), iterate() or release() is passed on once the nodes
     * that were initialised are released (every one of them, even if another release() throws).
     *
     * @return EXIT_SUCCESS, or EXIT_FAILURE if the engine is not configured
     */
    template <
        typename StopTokenType,
        typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<StopTokenType>, Pipeline::StopToken>>>
    int run(StopTokenType const& stop_token)
    {
        return run(Pipeline::StopToken{stop_token});
    }

    int run(Pipeline::StopToken stop_token)
    {
        if (!m_configured || m_running)
        {
            DUCTA_LOG_ERROR("DuctaEngine is not configured or already running");
            return EXIT_FAILURE;
        }
        /* Reset even when a node throws from init(), iterate() or release() */
        struct RunningGuard
        {
            bool& running;

            explicit RunningGuard(bool& flag) noexcept
                : running{flag}
            {
                running = true;
            }

            ~RunningGuard()
            {
                running = false;
            }
        } const guard{m_running};

        // Not PipelineEngine: its destructor (noexcept) could only log an exception of release()
        m_pipeline.init(); // releases the nodes it initialised if one init() throws
#if defined(__cpp_exceptions)
        try
        {
#endif
            while (!stop_token.stopRequested() && m_pipeline.iterate())
            {
            }
#if defined(__cpp_exceptions)
        }
        catch (...)
        {
            // Release the nodes, then pass on the exception of iterate()
            try
            {
                m_pipeline.release();
            }
            catch (...)
            {
            }
            throw;
        }
#endif
        m_pipeline.release(); // releases every node, then passes on the first exception
        return EXIT_SUCCESS;
    }

    bool configured() const noexcept
    {
        return m_configured;
    }

    /* Destroy the nodes (not allowed while running) */
    void clear() noexcept
    {
        if (m_running)
        {
            return;
        }
        m_configured = false;
        m_pipeline.clear();
        m_node_manager.clear();
    }

    /* Created nodes by name, e.g. to connect their inputs and outputs before run() */
    Pipeline::Nodes& nodes() noexcept
    {
        return m_node_manager.nodes();
    }

    Pipeline::NodeManager<Storage> const& node_manager() const noexcept
    {
        return m_node_manager;
    }

private:
    template <class Container>
    static bool contains(Container const& names, StringView name)
    {
        for (auto const& item : names)
        {
            if (StringView{item.data(), item.size()} == name)
            {
                return true;
            }
        }
        return false;
    }

private:
    /* Declaration order matters: the pipeline is destroyed (and disconnected from the nodes)
     * before the nodes */
    Pipeline::NodeManager<Storage> m_node_manager;
    Pipeline::Pipeline m_pipeline;
    bool m_configured{false};
    bool m_running{false};
};

} // namespace ducta

DEFINE_CONFIG(ducta::DuctaEngineConfig, ("nodes", nodes), ("pipeline", pipeline));

/* Both sections are required: without "pipeline" no node would ever run */
template <>
struct ducta::Configuration::ConfigRules<ducta::DuctaEngineConfig>
{
    static void validate(ducta::DuctaEngineConfig const& /*config*/, ::ducta::Configuration::Validator& validator)
    {
        validator.require("nodes");
        validator.require("pipeline");
    }
};

#endif // DUCTA_DUCTA_ENGINE_HPP
