// A sensor, a low-pass filter and a printer, created and connected from a JSON configuration.
// The printer and Ducta itself write through the logger: to Serial on the board, to the console
// on the computer.

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "ducta/Chrono/SteadyClock.hpp"
#include "ducta/Configuration/Configuration.hpp"
#include "ducta/DuctaEngine.hpp"
#include "ducta/IO/DefineInputs.hpp"
#include "ducta/IO/DefineOutputs.hpp"
#include "ducta/IO/TInput.hpp"
#include "ducta/IO/TOutput.hpp"
#include "ducta/Pipeline/DeclareNode.hpp"
#include "ducta/Utils/Log.hpp"

#if DUCTA_ENABLE_LOGGING
#if defined(ARDUINO)
#include "ducta/Utils/Log/PrintSink.hpp"
#else
#include "ducta/Utils/Log/ConsoleSink.hpp"
#endif
#endif

namespace app {

/* Simulated sensor: a saw-tooth signal */
struct Sensor
{
    ducta::IO::TOutput<float> value;
    int step{0};

    void init()
    {
    }

    void iterate()
    {
        value = static_cast<float>(step++ % 100);
    }

    void release()
    {
    }
};

struct LowPassConfig
{
    float alpha{0.5f};
};

struct LowPass
{
    using Config = LowPassConfig;

    explicit LowPass(Config const& config)
        : alpha{config.alpha}
    {
    }

    ducta::IO::TInput<float> in;
    ducta::IO::TOutput<float> out;
    float alpha;
    float state{0};

    void init()
    {
    }

    void iterate()
    {
        if (in.ready())
        {
            state += alpha * (in.value() - state);
            out = state;
        }
    }

    void release()
    {
    }
};

struct PrinterConfig
{
    int every{50};
};

struct Printer
{
    using Config = PrinterConfig;

    explicit Printer(Config const& config)
        : every{config.every}
    {
    }

    ducta::IO::TInput<float> in;
    int every;
    int frame{0};

    void init()
    {
    }

    void iterate()
    {
        if (in.ready())
        {
            float const value = in.value();
            if (++frame % every == 0)
            {
                DUCTA_LOG_INFO("frame {}: {}", frame, value);
            }
        }
    }

    void release()
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

DEFINE_CONFIG(app::LowPassConfig, ("alpha", alpha));
DEFINE_CONFIG(app::PrinterConfig, ("every", every));
DEFINE_NODE_OUTPUTS(app::Sensor, ("value", value));
DEFINE_NODE_INPUTS(app::LowPass, ("in", in));
DEFINE_NODE_OUTPUTS(app::LowPass, ("out", out));
DEFINE_NODE_INPUTS(app::Printer, ("in", in));
DECLARE_NODE(Sensor, app::Sensor);
DECLARE_NODE(LowPass, app::LowPass);
DECLARE_NODE(Printer, app::Printer);

namespace {

char const kConfig[] = R"({
  "nodes": {
    "sensor":  { "type": "Sensor" },
    "filter":  { "type": "LowPass", "config": { "alpha": 0.1 } },
    "printer": { "type": "Printer", "config": { "every": 25 } }
  },
  "pipeline": {
    "order": ["sensor", "filter", "printer"],
    "connections": [{ "sensor.value": "filter.in" }, { "filter.out": "printer.in" }],
    "target_frame_duration": "10ms"
  }
})";

/* Static: no heap, and the engine outlives setup() on Arduino */
ducta::Chrono::SteadyClock clock;
ducta::DuctaEngine<> engine{clock};

/* Without logging (DUCTA_ENABLE_LOGGING 0 in ducta_config.hpp) no sink is compiled in */
#if DUCTA_ENABLE_LOGGING
#if defined(ARDUINO)
ducta::Log::PrintSink log_sink{Serial};
#else
ducta::Log::ConsoleSink log_sink;
#endif
#endif

void start_logging()
{
#if DUCTA_ENABLE_LOGGING
    ducta::Log::logger().add_sink(log_sink);
    ducta::Log::logger().set_time_source(+[] {
        return ducta::Chrono::SteadyClock{}.now();
    });
#endif
}

bool configure()
{
    // Only needed while configuring
    static ducta::Configuration::PropTree tree;
    ducta::Configuration::Diagnostics diagnostics;
    if (!ducta::Configuration::loadJson(kConfig, tree, diagnostics) ||
        !engine.configure(tree, diagnostics))
    {
        for (auto const& diagnostic : diagnostics)
        {
            DUCTA_LOG_ERROR(
                "config: {}", ducta::StringView{diagnostic.message.data(), diagnostic.message.size()});
        }
        return false;
    }
    return true;
}

} // namespace

#if defined(ARDUINO)

void setup()
{
    Serial.begin(115200);
    start_logging();
    if (configure())
    {
        engine.run(app::Forever{}); // never returns
    }
}

void loop()
{
}

#else

int main()
{
    start_logging();
    return configure() ? engine.run(app::Forever{}) : 1;
}

#endif
