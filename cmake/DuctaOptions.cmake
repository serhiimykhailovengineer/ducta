# Compile-time capacities and features of Ducta (see include/ducta/Core/Capacity.hpp for their
# meaning and default values).
#
# Every option is a cache variable, so it can be set per preset or on the command line:
#   cmake --preset etl -DDUCTA_MAX_NODES=16 -DDUCTA_NODE_STORAGE_BYTES=2048
# An empty option (the default) keeps the value of the selected profile:
#   cmake --preset etl -DDUCTA_CAPACITY_PROFILE=mcu        # small capacities, no trace
# The values are passed to the code through the INTERFACE target ducta_options, which every
# Ducta library links. The embedded backend sizes its fixed containers with them; the standard
# backend grows dynamically and ignores most of them.

set(DUCTA_CAPACITY_PROFILE "default" CACHE STRING
    "Defaults of the capacities: 'default', or 'mcu' (small capacities, no execution trace)")
set_property(CACHE DUCTA_CAPACITY_PROFILE PROPERTY STRINGS default mcu)
if (NOT DUCTA_CAPACITY_PROFILE MATCHES "^(default|mcu)$")
    message(FATAL_ERROR "DUCTA_CAPACITY_PROFILE must be 'default' or 'mcu', got '${DUCTA_CAPACITY_PROFILE}'")
endif()

set(DUCTA_ENABLE_TRACING "" CACHE STRING
    "Execution trace: ON, OFF, or empty for the profile's default (ON, OFF for mcu)")
set_property(CACHE DUCTA_ENABLE_TRACING PROPERTY STRINGS "" ON OFF)

# ducta_capacity_option(<NAME> <DESCRIPTION>): empty keeps the profile's value
macro(ducta_capacity_option NAME DESCRIPTION)
    set(${NAME} "" CACHE STRING "${DESCRIPTION} (empty: the profile's value)")
    if (NOT "${${NAME}}" STREQUAL "")
        if (NOT "${${NAME}}" MATCHES "^[1-9][0-9]*$")
            message(FATAL_ERROR "${NAME} must be empty or a positive integer, got '${${NAME}}'")
        endif()
        list(APPEND DUCTA_CAPACITY_OPTIONS ${NAME})
    endif()
endmacro()

set(DUCTA_CAPACITY_OPTIONS)

# Pipeline
ducta_capacity_option(DUCTA_MAX_NODE_NAME_LENGTH "Maximum length of a node name (characters)")
ducta_capacity_option(DUCTA_MAX_NODES "Maximum number of nodes in a pipeline")
ducta_capacity_option(DUCTA_MAX_NODE_TYPES "Maximum number of node types registered with DECLARE_NODE")
ducta_capacity_option(DUCTA_NODE_STORAGE_BYTES "Size in bytes of the default static node storage (embedded backend)")
ducta_capacity_option(DUCTA_MAX_NODE_PORTS "Maximum number of inputs, outputs and events of one node (each)")
ducta_capacity_option(DUCTA_MAX_PORT_NAME_LENGTH "Maximum length of an input / output / event name (characters)")
ducta_capacity_option(DUCTA_MAX_CONNECTIONS "Maximum number of output -> input connections of a pipeline")
ducta_capacity_option(DUCTA_MAX_OUTPUT_CONNECTIONS "Maximum number of inputs connected to one output")
ducta_capacity_option(DUCTA_MAX_EVENT_CONNECTIONS "Maximum number of wired event emitters, and of receivers of one emitter")
ducta_capacity_option(DUCTA_MAX_EVENT_TYPES "Maximum number of distinct event types")
ducta_capacity_option(DUCTA_MAX_TRACED_FRAMES "Traced frames kept before flushing")
ducta_capacity_option(DUCTA_MAX_TRACED_EVENTS_PER_FRAME "Traced events recorded per frame")

# Configuration
ducta_capacity_option(DUCTA_CONFIG_MAX_NODES "Values and keys of one configuration tree")
ducta_capacity_option(DUCTA_CONFIG_MAX_KEY_LENGTH "Maximum length of a configuration key (node names used as keys too)")
ducta_capacity_option(DUCTA_CONFIG_MAX_STRING_LENGTH "Maximum length of a configuration string value")
ducta_capacity_option(DUCTA_CONFIG_MAX_PATH_LENGTH "Maximum length of a configuration path in diagnostics")
ducta_capacity_option(DUCTA_CONFIG_MAX_SOURCE_NAME_LENGTH "Maximum length of a configuration source (file) name")
ducta_capacity_option(DUCTA_CONFIG_MAX_DIAGNOSTICS "Diagnostics kept by one Diagnostics object")
ducta_capacity_option(DUCTA_CONFIG_MAX_JSON_TOKENS "JSON tokens of one document (values and keys); default 2 x DUCTA_CONFIG_MAX_NODES")

add_library(ducta_options INTERFACE)
if (DUCTA_CAPACITY_PROFILE STREQUAL "mcu")
    target_compile_definitions(ducta_options INTERFACE DUCTA_CAPACITY_PROFILE_MCU)
endif()
if (NOT "${DUCTA_ENABLE_TRACING}" STREQUAL "")
    if (DUCTA_ENABLE_TRACING)
        target_compile_definitions(ducta_options INTERFACE DUCTA_ENABLE_TRACING=1)
    else()
        target_compile_definitions(ducta_options INTERFACE DUCTA_ENABLE_TRACING=0)
    endif()
endif()
foreach(option IN LISTS DUCTA_CAPACITY_OPTIONS)
    target_compile_definitions(ducta_options INTERFACE ${option}=${${option}})
endforeach()

message(STATUS "Ducta capacity profile: ${DUCTA_CAPACITY_PROFILE}")
if (NOT "${DUCTA_ENABLE_TRACING}" STREQUAL "")
    message(STATUS "  DUCTA_ENABLE_TRACING = ${DUCTA_ENABLE_TRACING}")
endif()
foreach(option IN LISTS DUCTA_CAPACITY_OPTIONS)
    message(STATUS "  ${option} = ${${option}}")
endforeach()
