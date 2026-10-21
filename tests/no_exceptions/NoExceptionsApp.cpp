// Built with -fno-exceptions (as most firmware is): the whole engine, from the JSON configuration
// to run(), must compile and work without exceptions.

#if defined(__cpp_exceptions)
#error "this test must be compiled with -fno-exceptions"
#endif

#include <cstdio>
#include <cstdlib>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/DuctaEngine.hpp"
#include "ducta/IO/DeclareEvent.hpp"
#include "ducta/IO/DefineEvents.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TEvent.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/Pipeline/DeclareNode.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <boost/throw_exception.hpp> // declares the functions below (BOOST_NORETURN)

#include "ducta/Core/Panic.hpp"

/* Boost without exceptions (BOOST_NO_EXCEPTIONS) calls these: the application defines them */
namespace boost {
BOOST_NORETURN void throw_exception(std::exception const& e)
{
    ::ducta::fatal_error(e.what());
}

BOOST_NORETURN void throw_exception(std::exception const& e, boost::source_location const&)
{
    ::ducta::fatal_error(e.what());
}
} // namespace boost
#endif

namespace app {

struct Overflow
{
    int value{0};
};

struct Counter
{
    ducta::IO::TOutput<int> out;
    int count{0};

    void init()
    {
    }

    void release()
    {
    }

    void iterate()
    {
        out = ++count;
    }
};

struct GainConfig
{
    double gain{1.0};
};

struct Gain
{
    using Config = GainConfig;

    explicit Gain(Config const& config)
        : gain{config.gain}
    {
    }

    ducta::IO::TInput<int> in;
    ducta::IO::TOutput<int> out;
    ducta::IO::TEvent<Overflow> overflow;
    double gain;

    void init()
    {
    }

    void release()
    {
    }

    void iterate()
    {
        if (in.ready())
        {
            int const value = static_cast<int>(in.value() * gain);
            out = value;
            if (value > 10)
            {
                overflow(Overflow{value});
            }
        }
    }
};

struct Sink
{
    using SupportedEvents = ducta::EventList<Overflow>;

    ducta::IO::TInput<int> in;

    static int last;
    static int overflows;

    void init()
    {
    }

    void release()
    {
    }

    bool iterate()
    {
        if (in.ready())
        {
            last = in.value();
        }
        return last < 20; // stops the pipeline
    }

    void OnEvent(Overflow const&)
    {
        ++overflows;
    }
};

int Sink::last = 0;
int Sink::overflows = 0;

struct Clock
{
    using Timestamp = ducta::Chrono::TimestampUS;
    Timestamp time{0};

    Timestamp epoch()
    {
        return Timestamp{0};
    }

    Timestamp now()
    {
        return time;
    }

    void sleepFor(Timestamp duration)
    {
        time += duration;
    }
};

struct Forever
{
    bool stopRequested() const
    {
        return false;
    }
};

} // namespace app

DEFINE_CONFIG(app::GainConfig, ("gain", gain));
DEFINE_NODE_OUTPUTS(app::Counter, ("out", out));
DEFINE_NODE_INPUTS(app::Gain, ("in", in));
DEFINE_NODE_OUTPUTS(app::Gain, ("out", out));
DEFINE_NODE_EVENTS(app::Gain, ("overflow", overflow));
DEFINE_NODE_INPUTS(app::Sink, ("in", in));
DECLARE_NODE(Counter, app::Counter);
DECLARE_NODE(Gain, app::Gain);
DECLARE_NODE(Sink, app::Sink);

namespace {

char const kConfig[] = R"({
  "nodes": {
    "counter": { "type": "Counter" },
    "gain":    { "type": "Gain", "config": { "gain": 2.5 } },
    "sink":    { "type": "Sink" }
  },
  "pipeline": {
    "order": ["counter", "gain", "sink"],
    "connections": [{ "counter.out": "gain.in" }, { "gain.out": "sink.in" }],
    "target_frame_duration": "1ms"
  }
})";

int fail(char const* message)
{
    std::printf("FAILED: %s\n", message);
    return EXIT_FAILURE;
}

} // namespace

int main()
{
    ducta::Configuration::PropTree tree;
    ducta::Configuration::Diagnostics diagnostics;
    if (!ducta::Configuration::loadJson(kConfig, tree, diagnostics))
    {
        return fail("loadJson");
    }

    app::Clock clock;
    ducta::DuctaEngine<> engine{clock};
    if (!engine.configure(tree, diagnostics) || !diagnostics.empty())
    {
        return fail("configure");
    }
    if (engine.run(app::Forever{}) != EXIT_SUCCESS)
    {
        return fail("run");
    }

    // Counter 1, 2, ... * 2.5 = 2, 5, 7, 10, 12, 15, 17, 20: the sink stops the 8th frame,
    // 7 frames of 1ms were completed; 4 values above 10 were reported as events
    if (app::Sink::last != 20 || app::Sink::overflows != 4)
    {
        return fail("values");
    }
    if (clock.time != ducta::Chrono::TimestampUS{7000})
    {
        return fail("frame timing");
    }
    std::printf("OK\n");
    return EXIT_SUCCESS;
}
