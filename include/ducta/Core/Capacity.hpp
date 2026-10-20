/**
 * @file Capacity.hpp
 * @brief Compile-time capacities of the library, in one place
 *
 * Every value can be overridden with a compile definition. With CMake use the cache options
 * of the same name (see cmake/DuctaOptions.cmake), e.g.
 *     cmake --preset etl -DDUCTA_MAX_NODES=16 -DDUCTA_NODE_STORAGE_BYTES=2048
 *
 * The embedded backend uses them as the sizes of its fixed containers. The standard backend
 * grows dynamically and ignores most of them.
 *
 * Profiles: every default below is written as DUCTA_PROFILE_VALUE(<default>, <mcu>). Defining
 * DUCTA_CAPACITY_PROFILE_MCU (CMake: -DDUCTA_CAPACITY_PROFILE=mcu) selects the <mcu> values:
 * small capacities and no execution trace, for microcontrollers with tens of KB of RAM.
 * A value defined explicitly overrides the profile.
 */

#ifndef DUCTA_CORE_CAPACITY_HPP
#define DUCTA_CORE_CAPACITY_HPP

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

#endif // DUCTA_CORE_CAPACITY_HPP
