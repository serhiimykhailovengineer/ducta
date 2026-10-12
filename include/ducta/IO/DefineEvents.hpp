/**
 * @file DefineEvents.hpp
 * @brief Macros to define node events
 */

#ifndef DUCTA_DEFINE_EVENTS_HPP
#define DUCTA_DEFINE_EVENTS_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/IO/EventRef.hpp"

#include "ducta/Core/Preprocessor.hpp"

#define DUCTA_NODE_EVENTS_ELEM(node, i, elem)                 \
    DUCTA_COMMA_IF(i)                                         \
    ::ducta::pair<::ducta::StringView, ::ducta::IO::EventRef> \
    {                                                         \
        DUCTA_TUPLE_ELEM(2, 0, elem), ::ducta::IO::EventRef   \
        {                                                     \
            (node).DUCTA_TUPLE_ELEM(2, 1, elem)               \
        }                                                     \
    }

#define DUCTA_NODE_EVENTS_INIT(node, ...) \
    DUCTA_FOR_EACH_I(DUCTA_NODE_EVENTS_ELEM, node, __VA_ARGS__)

// Main macro to define events for a node type
#define DEFINE_NODE_EVENTS(NodeType, ...)                                                              \
    template <>                                                                                        \
    struct ducta::IO::NodeEventsTraits<DUCTA_PP_UNPAREN((NodeType))>                                   \
    {                                                                                                  \
        using Node = DUCTA_PP_UNPAREN((NodeType));                                                     \
        static ::ducta::Map<::ducta::StringView, ::ducta::IO::EventRef, DUCTA_MAX_NODE_PORTS> get(     \
            Node& node)                                                                                \
        {                                                                                              \
            return ::ducta::makeMap<::ducta::StringView, ::ducta::IO::EventRef, DUCTA_MAX_NODE_PORTS>( \
                DUCTA_NODE_EVENTS_INIT(node, __VA_ARGS__));                                            \
        }                                                                                              \
    };

#endif // DUCTA_DEFINE_EVENTS_HPP