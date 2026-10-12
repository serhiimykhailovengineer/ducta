/**
 * @file Capacity.hpp
 * @brief Compile-time capacities of the library, in one place
 *
 * Every value can be overridden with a compile definition. With CMake use the cache options
 * of the same name (see cmake/DuctaOptions.cmake), e.g.
 *     cmake --preset etl -DDUCTA_MAX_NODES=16 -DDUCTA_NODE_STORAGE_BYTES=2048
 *
 * The embedded backend uses them as the sizes of its fixed containers. The standard backend
 * grows dynamically and ignores most of them; the ones that also limit it are marked (std).
 */

#ifndef DUCTA_CORE_CAPACITY_HPP
#define DUCTA_CORE_CAPACITY_HPP

// ========== Pipeline ==========

// Maximum length of a node name (characters)
#ifndef DUCTA_MAX_NODE_NAME_LENGTH
#define DUCTA_MAX_NODE_NAME_LENGTH 50
#endif

// Maximum number of nodes in a pipeline (also in Nodes, NodeManager and the trace)
#ifndef DUCTA_MAX_NODES
#define DUCTA_MAX_NODES 50
#endif

// Maximum number of node types registered with DECLARE_NODE
#ifndef DUCTA_MAX_NODE_TYPES
#define DUCTA_MAX_NODE_TYPES 50
#endif

// Size in bytes of the default StaticNodeStorage (embedded backend): node sizes + alignment
#ifndef DUCTA_NODE_STORAGE_BYTES
#define DUCTA_NODE_STORAGE_BYTES 4096
#endif

// Maximum number of inputs, outputs and events of one node (each)
#ifndef DUCTA_MAX_NODE_PORTS
#define DUCTA_MAX_NODE_PORTS 25
#endif

// Maximum number of inputs connected to one output
#ifndef DUCTA_MAX_OUTPUT_CONNECTIONS
#define DUCTA_MAX_OUTPUT_CONNECTIONS 50
#endif

// Maximum number of wired event emitters in a pipeline, and of receivers of one emitter
#ifndef DUCTA_MAX_EVENT_CONNECTIONS
#define DUCTA_MAX_EVENT_CONNECTIONS 50
#endif

// Maximum number of distinct event types
#ifndef DUCTA_MAX_EVENT_TYPES
#define DUCTA_MAX_EVENT_TYPES 50
#endif

// Execution trace: frames kept before flushing, events recorded per frame
#ifndef DUCTA_MAX_TRACED_FRAMES
#define DUCTA_MAX_TRACED_FRAMES 100
#endif

#ifndef DUCTA_MAX_TRACED_EVENTS_PER_FRAME
#define DUCTA_MAX_TRACED_EVENTS_PER_FRAME 100
#endif

// ========== Configuration ==========

// Nodes (values and keys) of one PropTree
#ifndef DUCTA_CONFIG_MAX_NODES
#define DUCTA_CONFIG_MAX_NODES 128
#endif

// Maximum length of an object key in a PropTree (a node name used as a key too)
#ifndef DUCTA_CONFIG_MAX_KEY_LENGTH
#define DUCTA_CONFIG_MAX_KEY_LENGTH 32
#endif

// Maximum length of a string value in a PropTree
#ifndef DUCTA_CONFIG_MAX_STRING_LENGTH
#define DUCTA_CONFIG_MAX_STRING_LENGTH 64
#endif

// Maximum length of a path in diagnostics, e.g. "nodes.Gain.gain"
#ifndef DUCTA_CONFIG_MAX_PATH_LENGTH
#define DUCTA_CONFIG_MAX_PATH_LENGTH 128
#endif

// Maximum length of the source (file) name of a PropTree
#ifndef DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH
#define DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH 64
#endif

// Diagnostics kept by one Diagnostics object (the rest are counted as dropped)
#ifndef DUCTA_CONFIG_MAX_DIAGNOSTICS
#define DUCTA_CONFIG_MAX_DIAGNOSTICS 16
#endif

// JSON tokens of one document (values and keys)
#ifndef DUCTA_CONFIG_MAX_JSON_TOKENS
#define DUCTA_CONFIG_MAX_JSON_TOKENS (2 * DUCTA_CONFIG_MAX_NODES)
#endif

#endif // DUCTA_CORE_CAPACITY_HPP
