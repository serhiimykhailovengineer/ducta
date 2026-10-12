#ifndef DUCTA_IO_DECLAREEVENT_HPP
#define DUCTA_IO_DECLAREEVENT_HPP

#include "ducta/Core/Types/TypeRegistry.hpp"

namespace ducta {
namespace Private {
template<class T>
struct EventNameOverride
{
    static constexpr bool has_override = false;
};

template<class T>
constexpr StringView type_name()
{
    using EventType = ::ducta::decay_t<T>;

    if constexpr (EventNameOverride<EventType>::has_override)
        return EventNameOverride<EventType>::value;
    else
        return auto_type_name<EventType>();
}

template<class T>
TypeId type_id_impl(StringView file, int line)
{
    using EventType = ::ducta::decay_t<T>;

    static const TypeId id = TypeRegistry::instance().register_or_get(
        type_name<EventType>(),
        TypeFingerprint{ sizeof(EventType), alignof(EventType) },
        file, line);

    return id;
}

} // namespace Private

template<class... Events>
struct EventList {};

template<class T, class = void>
struct SupportedEventsOf
{
    using type = EventList<>; // fallback: нода не задекларувала жодних подій
};

template<class T>
struct SupportedEventsOf<T, ::ducta::void_t<typename T::SupportedEvents>>
{
    using type = typename T::SupportedEvents; // нода явно оголосила список
};

} // namespace ducta


#define DUCTA_TYPE_ID(T) ::ducta::Private::type_id_impl<T>(__FILE__, __LINE__)

#define DUCTA_DECLARE_EVENT_NAME(Type, NameLiteral)            \
    template<>                                                 \
    struct ::ducta::Private::EventNameOverride<Type>           \
    {                                                          \
        static constexpr bool has_override = true;             \
        static constexpr StringView value = NameLiteral;       \
    }

#endif // DUCTA_IO_DECLAREEVENT_HPP