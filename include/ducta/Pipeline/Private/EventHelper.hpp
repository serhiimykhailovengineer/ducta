#ifndef DUCTA_PIPELINE_PRIVATE_EVENTHELPER_HPP
#define DUCTA_PIPELINE_PRIVATE_EVENTHELPER_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Types/Array.hpp"
#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"

#include "ducta/IO/DeclareEvent.hpp"

namespace ducta {
namespace Pipeline {
namespace Private {

template <class NodeType, class EventType, class = void>
struct HasExactOnEvent : ::ducta::false_type
{
};

template <class NodeType, class EventType>
struct HasExactOnEvent<
    NodeType,
    EventType,
    ::ducta::void_t<decltype(static_cast<void (NodeType::*)(EventType const&)>(&NodeType::OnEvent))>>
    : ::ducta::true_type
{
};

struct EventEntry
{
    ::ducta::Private::TypeId id;
    Expected<bool, Error> (*fn)(void*, void*);
};

template <class NodeType, class EventType>
constexpr auto make_event_entry()
{
    static_assert(
        HasExactOnEvent<NodeType, EventType>::value,
        "Node declared EventType in SupportedEvents but has no exact "
        "'void OnEvent(EventType const&)' overload");

    return EventEntry{DUCTA_TYPE_ID(EventType), +[](void* obj, void* event) -> Expected<bool, Error> {
                          NodeType& node = *static_cast<NodeType*>(obj);
                          EventType const& ev = *static_cast<EventType const*>(event);
                          node.OnEvent(ev);
                          return true;
                      }};
}

template <class NodeType, class... Events>
constexpr auto make_event_table(EventList<Events...>)
{
    return ::ducta::Array<EventEntry, sizeof...(Events)>{make_event_entry<NodeType, Events>()...};
}

template <class... Events>
constexpr auto event_ids_of(EventList<Events...>)
{
    return ::ducta::Array<::ducta::Private::TypeId, sizeof...(Events)>{DUCTA_TYPE_ID(Events)...};
}

template <class Table>
Expected<bool, Error> call_event(void* obj, ::ducta::Private::TypeId id, void* event, Table const& table)
{
    for (auto const& entry : table)
    {
        if (entry.id == id)
        {
            return entry.fn(obj, event);
        }
    }
    return true;
}

} // namespace Private
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_PRIVATE_EVENTHELPER_HPP