/**
 * @file Capacity.hpp
 * @brief Compile-time capacities of the library, in one place
 *
 * Every value can be overridden in ducta_config.hpp (see Config.hpp) or with a compile
 * definition. With CMake use the cache options of the same name (see cmake/DuctaOptions.cmake):
 *     cmake --preset etl -DDUCTA_MAX_NODES=16 -DDUCTA_NODE_STORAGE_BYTES=2048
 *
 * The embedded backend uses them as the sizes of its fixed containers. The standard backend
 * grows dynamically and ignores most of them.
 *
 * Profiles: every default below is written as DUCTA_PROFILE_VALUE(<default>, <mcu>). Defining
 * DUCTA_CAPACITY_PROFILE_MCU (CMake: -DDUCTA_CAPACITY_PROFILE=mcu) selects the <mcu> values:
 * small capacities and no execution trace, for microcontrollers with tens of KB of RAM.
 * A value defined explicitly overrides the profile.
 *
 * The capacities change the layout of Ducta types, so the whole program must use the same ones.
 * Every source file that includes Ducta checks this at startup (before main()) and calls
 * ducta::fatal_error if its capacities differ from the ones of another file. DUCTA_NO_CAPACITY_CHECK
 * removes the check.
 */

#ifndef DUCTA_CORE_CAPACITY_HPP
#define DUCTA_CORE_CAPACITY_HPP

#include "ducta/Core/Config.hpp"

#if defined(DUCTA_CAPACITY_PROFILE_MCU)
#define DUCTA_PROFILE_VALUE(default_value, mcu_value) mcu_value
#else
#define DUCTA_PROFILE_VALUE(default_value, mcu_value) default_value
#endif

// ========== Features ==========

// Execution trace (PipelineConfig::should_trace_execution). 0 removes the trace buffers
// (DUCTA_MAX_TRACED_FRAMES frames of DUCTA_MAX_NODES durations and
// DUCTA_MAX_TRACED_EVENTS_PER_FRAME events) and the trace code from the pipeline
#ifndef DUCTA_ENABLE_TRACING
#define DUCTA_ENABLE_TRACING DUCTA_PROFILE_VALUE(1, 0)
#endif

// ========== Pipeline ==========

// Maximum length of a node name (characters)
#ifndef DUCTA_MAX_NODE_NAME_LENGTH
#define DUCTA_MAX_NODE_NAME_LENGTH DUCTA_PROFILE_VALUE(50, 16)
#endif

// Maximum number of nodes in a pipeline (also in Nodes, NodeManager and the trace)
#ifndef DUCTA_MAX_NODES
#define DUCTA_MAX_NODES DUCTA_PROFILE_VALUE(50, 16)
#endif

// Maximum number of node types registered with DECLARE_NODE
#ifndef DUCTA_MAX_NODE_TYPES
#define DUCTA_MAX_NODE_TYPES DUCTA_PROFILE_VALUE(50, 16)
#endif

// Size in bytes of the default StaticNodeStorage (embedded backend): node sizes + alignment
#ifndef DUCTA_NODE_STORAGE_BYTES
#define DUCTA_NODE_STORAGE_BYTES DUCTA_PROFILE_VALUE(4096, 2048)
#endif

// Maximum number of inputs, outputs and events of one node (each)
#ifndef DUCTA_MAX_NODE_PORTS
#define DUCTA_MAX_NODE_PORTS DUCTA_PROFILE_VALUE(25, 4)
#endif

// Maximum length of an input / output / event name of a node (characters)
#ifndef DUCTA_MAX_PORT_NAME_LENGTH
#define DUCTA_MAX_PORT_NAME_LENGTH DUCTA_PROFILE_VALUE(32, 12)
#endif

// Maximum number of output -> input connections of a pipeline (PipelineConfig::connections)
#ifndef DUCTA_MAX_CONNECTIONS
#define DUCTA_MAX_CONNECTIONS DUCTA_PROFILE_VALUE(50, 24)
#endif

// Maximum number of inputs connected to one output
#ifndef DUCTA_MAX_OUTPUT_CONNECTIONS
#define DUCTA_MAX_OUTPUT_CONNECTIONS DUCTA_PROFILE_VALUE(50, 4)
#endif

// Maximum number of wired event emitters in a pipeline, and of receivers of one emitter
#ifndef DUCTA_MAX_EVENT_CONNECTIONS
#define DUCTA_MAX_EVENT_CONNECTIONS DUCTA_PROFILE_VALUE(50, 8)
#endif

// Maximum number of distinct event types
#ifndef DUCTA_MAX_EVENT_TYPES
#define DUCTA_MAX_EVENT_TYPES DUCTA_PROFILE_VALUE(50, 8)
#endif

// Execution trace: frames kept before flushing, events recorded per frame
#ifndef DUCTA_MAX_TRACED_FRAMES
#define DUCTA_MAX_TRACED_FRAMES DUCTA_PROFILE_VALUE(100, 4)
#endif

#ifndef DUCTA_MAX_TRACED_EVENTS_PER_FRAME
#define DUCTA_MAX_TRACED_EVENTS_PER_FRAME DUCTA_PROFILE_VALUE(100, 8)
#endif

// ========== Configuration ==========

// Nodes (values and keys) of one PropTree
#ifndef DUCTA_CONFIG_MAX_NODES
#define DUCTA_CONFIG_MAX_NODES DUCTA_PROFILE_VALUE(128, 96)
#endif

// Maximum length of an object key in a PropTree (a node name used as a key too)
#ifndef DUCTA_CONFIG_MAX_KEY_LENGTH
#define DUCTA_CONFIG_MAX_KEY_LENGTH DUCTA_PROFILE_VALUE(32, 24)
#endif

// Maximum length of a string value in a PropTree
#ifndef DUCTA_CONFIG_MAX_STRING_LENGTH
#define DUCTA_CONFIG_MAX_STRING_LENGTH DUCTA_PROFILE_VALUE(64, 24)
#endif

// Maximum length of a path in diagnostics, e.g. "nodes.Gain.gain"
#ifndef DUCTA_CONFIG_MAX_PATH_LENGTH
#define DUCTA_CONFIG_MAX_PATH_LENGTH DUCTA_PROFILE_VALUE(128, 48)
#endif

// Maximum length of the source (file) name of a PropTree
#ifndef DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH
#define DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH DUCTA_PROFILE_VALUE(64, 16)
#endif

// Diagnostics kept by one Diagnostics object (the rest are counted as dropped)
#ifndef DUCTA_CONFIG_MAX_DIAGNOSTICS
#define DUCTA_CONFIG_MAX_DIAGNOSTICS DUCTA_PROFILE_VALUE(16, 4)
#endif

// JSON tokens of one document (values and keys)
#ifndef DUCTA_CONFIG_MAX_JSON_TOKENS
#define DUCTA_CONFIG_MAX_JSON_TOKENS (2 * DUCTA_CONFIG_MAX_NODES)
#endif

// ========== Logging (see ducta/Utils/Log.hpp) ==========

// 0 removes logging: the DUCTA_LOG_* calls compile to nothing and the logger does nothing
#ifndef DUCTA_ENABLE_LOGGING
#define DUCTA_ENABLE_LOGGING 1
#endif

// Sinks attached to the logger at the same time
#ifndef DUCTA_MAX_LOG_SINKS
#define DUCTA_MAX_LOG_SINKS DUCTA_PROFILE_VALUE(4, 2)
#endif

// Maximum length of a log message (characters, longer ones are truncated)
#ifndef DUCTA_LOG_MESSAGE_LENGTH
#define DUCTA_LOG_MESSAGE_LENGTH DUCTA_PROFILE_VALUE(256, 96)
#endif

// Least important level compiled in: 0 debug, 1 info, 2 warning, 3 error, 4 nothing. The calls
// below it are removed from the program (the logger also filters at run time)
#ifndef DUCTA_LOG_LEVEL
#define DUCTA_LOG_LEVEL DUCTA_PROFILE_VALUE(0, 1)
#endif

// ========== The same capacities in the whole program ==========

#if !defined(DUCTA_NO_CAPACITY_CHECK)

#include <cstdint>

#include "ducta/Core/Panic.hpp"

namespace ducta {
namespace Private {

constexpr uint64_t signature_of() noexcept
{
    return 0xcbf29ce484222325ULL;
}

/* FNV-1a style mix of the values */
template <class... Values>
constexpr uint64_t signature_of(uint64_t value, Values... values) noexcept
{
    return (signature_of(values...) ^ value) * 0x100000001b3ULL;
}

/* Backend and capacities of this source file */
constexpr uint64_t capacities_signature = signature_of(
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
    1,
#else
    2,
#endif
    DUCTA_ENABLE_TRACING,
    DUCTA_MAX_NODE_NAME_LENGTH,
    DUCTA_MAX_NODES,
    DUCTA_MAX_NODE_TYPES,
    DUCTA_NODE_STORAGE_BYTES,
    DUCTA_MAX_NODE_PORTS,
    DUCTA_MAX_PORT_NAME_LENGTH,
    DUCTA_MAX_CONNECTIONS,
    DUCTA_MAX_OUTPUT_CONNECTIONS,
    DUCTA_MAX_EVENT_CONNECTIONS,
    DUCTA_MAX_EVENT_TYPES,
    DUCTA_MAX_TRACED_FRAMES,
    DUCTA_MAX_TRACED_EVENTS_PER_FRAME,
    DUCTA_CONFIG_MAX_NODES,
    DUCTA_CONFIG_MAX_KEY_LENGTH,
    DUCTA_CONFIG_MAX_STRING_LENGTH,
    DUCTA_CONFIG_MAX_PATH_LENGTH,
    DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH,
    DUCTA_CONFIG_MAX_DIAGNOSTICS,
    DUCTA_CONFIG_MAX_JSON_TOKENS,
    DUCTA_ENABLE_LOGGING,
    DUCTA_MAX_LOG_SINKS,
    DUCTA_LOG_MESSAGE_LENGTH);

/* Of the program: the linker keeps one definition (initialized before any dynamic
 * initialization) for all source files, so a file with other capacities sees another value */
inline uint64_t program_capacities_signature = capacities_signature;

} // namespace Private
} // namespace ducta

namespace {
/* Internal linkage: every source file runs its own check (an inline function shared by the
 * files would compare one signature only), before main() */
struct DuctaCapacitiesCheck
{
    DuctaCapacitiesCheck()
    {
        // Volatile: the compiler must not assume the value of this file's initializer
        uint64_t const program_signature =
            *static_cast<uint64_t const volatile*>(&::ducta::Private::program_capacities_signature);
        if (program_signature != ::ducta::Private::capacities_signature)
        {
            ::ducta::fatal_error(
                "Ducta capacities or backend differ between source files of the program: "
                "configure them in ducta_config.hpp, seen by every file that includes Ducta");
        }
    }
};

DuctaCapacitiesCheck const ducta_capacities_check;
} // namespace

#endif // DUCTA_NO_CAPACITY_CHECK

#endif // DUCTA_CORE_CAPACITY_HPP
