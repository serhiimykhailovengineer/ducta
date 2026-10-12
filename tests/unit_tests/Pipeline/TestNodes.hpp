#pragma once

#include "ducta/Configuration/Configuration.hpp"
#include "ducta/Pipeline/DeclareNode.hpp"

#include "ducta/Core/Types/String.hpp"

#include <string>

// Nodes shared by NodeFactoryTest and NodeManagerTest: a DECLARE_NODE name is unique in the
// whole program, so the declarations live in one header (registered once per program).
namespace TestNodes {

struct GainConfig
{
    double gain{1.0};
    ducta::String<16> label;
};

struct GainNode
{
    using Config = GainConfig;

    static inline double last_gain = 0.0;

    explicit GainNode(Config const& config)
        : config{config}
    {
        last_gain = config.gain;
    }

    void init()
    {
    }

    void release()
    {
    }

    Config config;
};

struct PlainNode
{
    void init()
    {
    }

    void release()
    {
    }

    int value{42};
};

struct CountingNode
{
    static inline int alive = 0;

    CountingNode()
    {
        ++alive;
    }

    ~CountingNode()
    {
        --alive;
    }

    void init()
    {
    }

    void release()
    {
    }
};

struct AlignedNode
{
    void init()
    {
    }

    void release()
    {
    }

    alignas(64) unsigned char data[8]{};
};

template <class A, class B>
struct PairNode
{
    void init()
    {
    }

    void release()
    {
    }

    A first{};
    B second{};
};

/* Records init / iterate / release calls of LoggingNode: "iA" = init of node 'A' */
inline std::string& run_log()
{
    static std::string log;
    return log;
}

template <char Tag>
struct LoggingNode
{
    void init()
    {
        run_log() += 'i';
        run_log() += Tag;
    }

    void iterate()
    {
        run_log() += Tag;
    }

    void release()
    {
        run_log() += 'r';
        run_log() += Tag;
    }
};

} // namespace TestNodes

DEFINE_CONFIG(TestNodes::GainConfig, ("gain", gain), ("label", label));
DEFINE_CONFIG_RULES(TestNodes::GainConfig, ("gain", config.gain > 0.0, "must be positive"));

DECLARE_NODE(Gain, TestNodes::GainNode);
DECLARE_NODE(Plain, TestNodes::PlainNode);
DECLARE_NODE(Counting, TestNodes::CountingNode);
DECLARE_NODE(Aligned, TestNodes::AlignedNode);
DECLARE_NODE(First, TestNodes::LoggingNode<'A'>);
DECLARE_NODE(Second, TestNodes::LoggingNode<'B'>);
// Template arguments may contain commas
DECLARE_NODE(IntFloatPair, TestNodes::PairNode<int, float>);
