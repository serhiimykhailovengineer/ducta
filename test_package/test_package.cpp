// Two nodes created from a JSON configuration, run for a few frames

#include <cstdio>
#include <cstdlib>

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/DuctaEngine.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/Pipeline/DeclareNode.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
/* Platform of the embedded backend: provided by the application (here: nothing to wait for) */
extern "C" ETL_CHRONO_STEADY_CLOCK_DURATION::rep etl_get_steady_clock()
{
    return 0;
}

extern "C" void ducta_sleep_for_us(etl::chrono::microseconds::rep)
{
}
#endif

namespace app {

struct CounterConfig
{
    int step{1};
};

struct Counter
{
    using Config = CounterConfig;

    explicit Counter(Config const& config)
        : step{config.step}
    {
    }

    ducta::IO::TOutput<int> out;
    int step;
    int value{0};

    void init()
    {
    }

    void iterate()
    {
        value += step;
        out = value;
    }

    void release()
    {
    }
};

struct Sink
{
    ducta::IO::TInput<int> in;
    static int last;

    void init()
    {
    }

    bool iterate()
    {
        if (in.ready())
        {
            last = in.value();
        }
        return last < 10; // stops the pipeline
    }

    void release()
    {
    }
};

int Sink::last = 0;

struct Clock
{
    using Timestamp = ducta::Chrono::TimestampUS;

    Timestamp epoch()
    {
        return Timestamp{0};
    }

    Timestamp now()
    {
        return Timestamp{0};
    }

    void sleepFor(Timestamp)
    {
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

DEFINE_CONFIG(app::CounterConfig, ("step", step));
DEFINE_NODE_OUTPUTS(app::Counter, ("out", out));
DEFINE_NODE_INPUTS(app::Sink, ("in", in));
DECLARE_NODE(Counter, app::Counter);
DECLARE_NODE(Sink, app::Sink);

int main()
{
    char const config[] = R"({
        "nodes": {
            "counter": { "type": "Counter", "config": { "step": 2 } },
            "sink": { "type": "Sink" }
        },
        "pipeline": { "order": ["counter", "sink"], "connections": [{ "counter.out": "sink.in" }] }
    })";

    ducta::Configuration::PropTree tree;
    ducta::Configuration::Diagnostics diagnostics;
    app::Clock clock;
    ducta::DuctaEngine<> engine{clock};
    if (!ducta::Configuration::loadJson(config, tree, diagnostics) ||
        !engine.configure(tree, diagnostics) || engine.run(app::Forever{}) != EXIT_SUCCESS ||
        app::Sink::last != 10)
    {
        std::printf("ducta test_package: FAILED\n");
        return EXIT_FAILURE;
    }
    std::printf("ducta test_package: OK\n");
    return EXIT_SUCCESS;
}
