#pragma once

// Nodes created by DuctaEngine from a configuration (see DuctaEngineTest.cpp)

#include "ducta/Configuration/DefineConfig.hpp"
#include "ducta/IO/DefineEvents.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TEvent.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/Pipeline/DeclareNode.hpp"

#include "Adder.hpp"
#include "Multiplier.hpp"

namespace ducta {
namespace Test {

/* Outputs a configured value every iteration */
struct ConstantConfig
{
    int value{0};
};

class Constant
{
public:
    using Config = ConstantConfig;

    IO::TOutput<int> out;

    explicit Constant(Config const& config)
        : m_value{config.value}
    {
    }

    void init()
    {
    }

    void iterate()
    {
        out = m_value;
    }

    void release()
    {
    }

private:
    int m_value;
};

/* Multiplies its input by a configured factor */
struct ScaleConfig
{
    int factor{1};
};

class Scale
{
public:
    using Config = ScaleConfig;

    IO::TInput<int> in;
    IO::TOutput<int> out;

    explicit Scale(Config const& config)
        : m_factor{config.factor}
    {
    }

    void init()
    {
    }

    void iterate()
    {
        if (in.ready())
        {
            out = in.value() * m_factor;
        }
    }

    void release()
    {
    }

private:
    int m_factor;
};

/* Counts its iterations and stops the pipeline after the configured number */
struct CountdownConfig
{
    int iterations{1};
};

class Countdown
{
public:
    using Config = CountdownConfig;

    IO::TOutput<int> count;

    explicit Countdown(Config const& config)
        : m_iterations{config.iterations}
    {
    }

    void init()
    {
        m_count = 0;
    }

    bool iterate()
    {
        count = ++m_count;
        return m_count < m_iterations;
    }

    void release()
    {
    }

private:
    int m_iterations;
    int m_count{0};
};

/* Event sent by Ticker to Accumulator */
struct Tick
{
    int value;
};

/* Emits Tick{step} every iteration */
struct TickerConfig
{
    int step{1};
};

class Ticker
{
public:
    using Config = TickerConfig;

    IO::TEvent<Tick> tick;

    explicit Ticker(Config const& config)
        : m_step{config.step}
    {
    }

    void init()
    {
    }

    void iterate()
    {
        tick(Tick{m_step});
    }

    void release()
    {
    }

private:
    int m_step;
};

/* Sums the values of all received Tick events */
class Accumulator
{
public:
    using SupportedEvents = EventList<Tick>;

    IO::TOutput<int> total;

    void init()
    {
        m_total = 0;
    }

    void OnEvent(Tick const& tick)
    {
        m_total += tick.value;
    }

    void iterate()
    {
        total = m_total;
    }

    void release()
    {
    }

private:
    int m_total{0};
};

} // namespace Test
} // namespace ducta

DEFINE_CONFIG(ducta::Test::ConstantConfig, ("value", value));
DEFINE_CONFIG(ducta::Test::ScaleConfig, ("factor", factor));
DEFINE_CONFIG(ducta::Test::CountdownConfig, ("iterations", iterations));
DEFINE_CONFIG_RULES(
    ducta::Test::CountdownConfig,
    ("iterations", config.iterations > 0, "must be positive"));
DEFINE_CONFIG(ducta::Test::TickerConfig, ("step", step));

DEFINE_NODE_OUTPUTS(ducta::Test::Constant, ("out", out));
DEFINE_NODE_INPUTS(ducta::Test::Scale, ("in", in));
DEFINE_NODE_OUTPUTS(ducta::Test::Scale, ("out", out));
DEFINE_NODE_OUTPUTS(ducta::Test::Countdown, ("count", count));
DEFINE_NODE_EVENTS(ducta::Test::Ticker, ("tick", tick));
DEFINE_NODE_OUTPUTS(ducta::Test::Accumulator, ("total", total));

DECLARE_NODE(Constant, ducta::Test::Constant);
DECLARE_NODE(Scale, ducta::Test::Scale);
DECLARE_NODE(Countdown, ducta::Test::Countdown);
DECLARE_NODE(Ticker, ducta::Test::Ticker);
DECLARE_NODE(Accumulator, ducta::Test::Accumulator);
DECLARE_NODE(Add, ducta::Test::Adder<int>);
DECLARE_NODE(Multiply, ducta::Test::Multiplier<int>);
