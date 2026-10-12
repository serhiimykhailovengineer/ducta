#ifndef DUCTA_EVENT_REF_HPP
#define DUCTA_EVENT_REF_HPP

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/FunctionRef.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/TypeRegistry.hpp"
#include "ducta/IO/DeclareEvent.hpp"

namespace ducta {
namespace IO {

class EventRef
{
public:
    using EventDelegate = FunctionRef<void(void*)>;

private:
    struct VTable {
        void (*subscribe)(void*, EventDelegate);
        void (*unsubscribe)(void*);
        ::ducta::Private::TypeId (*type_id)(void*);
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj, EventDelegate delegate) {
                static_cast<T*>(obj)->subscribe(delegate);
            },
            +[](void* obj) {
                static_cast<T*>(obj)->unsubscribe();
            },
            +[](void*) {
                return DUCTA_TYPE_ID(typename T::EventType);
            }
        };
        return vt;
    }

public:
    template <typename TEventType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<TEventType>, EventRef>>>
    explicit EventRef(TEventType& event)
        : m_event_object(&event)
        , m_vtable(&vt_for<::ducta::decay_t<TEventType>>())
    {}

    ::ducta::Private::TypeId type_id() const
    {
        return m_vtable->type_id(m_event_object);
    }
    void subscribe(EventDelegate delegate)
    {
        m_vtable->subscribe(m_event_object, delegate);
    }
    void unsubscribe()
    {
        m_vtable->unsubscribe(m_event_object);
    }

private:
    void* m_event_object;
    const VTable* m_vtable;
};

/* Default: node with no DEFINE_NODE_EVENTS has no events. */
template <typename Node>
struct NodeEventsTraits
{
    static ::ducta::Map<::ducta::StringView, EventRef, 25> get(Node&) { return {}; }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_EVENT_REF_HPP
