// A sensor, a low-pass filter and a printer, created and connected from a JSON configuration

#include <stdio.h>

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

namespace app {

void print_line(char const* text)
{
#if defined(ARDUINO)
    Serial.println(text);
#else
    puts(text);
    fflush(stdout);
#endif
}

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
                char line[48];
                snprintf(
                    line,
                    sizeof(line),
                    "frame %d: %d.%02d",
                    frame,
                    static_cast<int>(value),
                    static_cast<int>(value * 100) % 100);
                print_line(line);
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
            char line[128];
            snprintf(
                line,
                sizeof(line),
                "config: %.*s",
                static_cast<int>(diagnostic.message.size()),
                diagnostic.message.data());
            app::print_line(line);
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
    return configure() ? engine.run(app::Forever{}) : 1;
}

#endif
