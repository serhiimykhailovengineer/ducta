# Compile-time capacities of Ducta (see include/ducta/Core/Capacity.hpp for their meaning).
#
# Every option is a cache variable, so it can be set per preset or on the command line:
#   cmake --preset etl -DDUCTA_MAX_NODES=16 -DDUCTA_NODE_STORAGE_BYTES=2048
# The values are passed to the code through the INTERFACE target ducta_options, which every
# Ducta library links. The embedded backend sizes its fixed containers with them; the standard
# backend grows dynamically and ignores most of them.

# ducta_capacity_option(<NAME> <DEFAULT> <DESCRIPTION>)
macro(ducta_capacity_option NAME DEFAULT DESCRIPTION)
    set(${NAME} ${DEFAULT} CACHE STRING "${DESCRIPTION}")
    if (NOT "${${NAME}}" MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "${NAME} must be a positive integer, got '${${NAME}}'")
    endif()
    list(APPEND DUCTA_CAPACITY_OPTIONS ${NAME})
endmacro()

set(DUCTA_CAPACITY_OPTIONS)

# Pipeline
ducta_capacity_option(DUCTA_MAX_NODE_NAME_LENGTH 50 "Maximum length of a node name (characters)")
ducta_capacity_option(DUCTA_MAX_NODES 50 "Maximum number of nodes in a pipeline")
ducta_capacity_option(DUCTA_MAX_NODE_TYPES 50 "Maximum number of node types registered with DECLARE_NODE")
ducta_capacity_option(DUCTA_NODE_STORAGE_BYTES 4096 "Size in bytes of the default static node storage (embedded backend)")
ducta_capacity_option(DUCTA_MAX_NODE_PORTS 25 "Maximum number of inputs, outputs and events of one node (each)")
ducta_capacity_option(DUCTA_MAX_PORT_NAME_LENGTH 32 "Maximum length of an input / output / event name (characters)")
ducta_capacity_option(DUCTA_MAX_CONNECTIONS 50 "Maximum number of output -> input connections of a pipeline")
ducta_capacity_option(DUCTA_MAX_OUTPUT_CONNECTIONS 50 "Maximum number of inputs connected to one output")
ducta_capacity_option(DUCTA_MAX_EVENT_CONNECTIONS 50 "Maximum number of wired event emitters, and of receivers of one emitter")
ducta_capacity_option(DUCTA_MAX_EVENT_TYPES 50 "Maximum number of distinct event types")
ducta_capacity_option(DUCTA_MAX_TRACED_FRAMES 100 "Traced frames kept before flushing")
ducta_capacity_option(DUCTA_MAX_TRACED_EVENTS_PER_FRAME 100 "Traced events recorded per frame")

# Configuration
ducta_capacity_option(DUCTA_CONFIG_MAX_NODES 128 "Values and keys of one configuration tree")
ducta_capacity_option(DUCTA_CONFIG_MAX_KEY_LENGTH 32 "Maximum length of a configuration key (node names used as keys too)")
ducta_capacity_option(DUCTA_CONFIG_MAX_STRING_LENGTH 64 "Maximum length of a configuration string value")
ducta_capacity_option(DUCTA_CONFIG_MAX_PATH_LENGTH 128 "Maximum length of a configuration path in diagnostics")
ducta_capacity_option(DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH 64 "Maximum length of a configuration source (file) name")
ducta_capacity_option(DUCTA_CONFIG_MAX_DIAGNOSTICS 16 "Diagnostics kept by one Diagnostics object")

# JSON tokens of one document (values and keys); empty means 2 x DUCTA_CONFIG_MAX_NODES
set(DUCTA_CONFIG_MAX_JSON_TOKENS "" CACHE STRING
    "JSON tokens of one document (values and keys); empty: 2 x DUCTA_CONFIG_MAX_NODES")
if (NOT "${DUCTA_CONFIG_MAX_JSON_TOKENS}" STREQUAL "")
    if (NOT "${DUCTA_CONFIG_MAX_JSON_TOKENS}" MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
            "DUCTA_CONFIG_MAX_JSON_TOKENS must be empty or a positive integer, got '${DUCTA_CONFIG_MAX_JSON_TOKENS}'")
    endif()
    list(APPEND DUCTA_CAPACITY_OPTIONS DUCTA_CONFIG_MAX_JSON_TOKENS)
endif()

add_library(ducta_options INTERFACE)
foreach(option IN LISTS DUCTA_CAPACITY_OPTIONS)
    target_compile_definitions(ducta_options INTERFACE ${option}=${${option}})
endforeach()

message(STATUS "Ducta capacities:")
foreach(option IN LISTS DUCTA_CAPACITY_OPTIONS)
    message(STATUS "  ${option} = ${${option}}")
endforeach()
