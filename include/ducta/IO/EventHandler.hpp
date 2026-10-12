#ifndef DUCTA_IO_EVENT_HANDLER_HPP
#define DUCTA_IO_EVENT_HANDLER_HPP

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/FunctionRef.hpp"

namespace ducta {
namespace IO {

namespace Private {
template <class T, class E>
E event_arg_of(void (T::*)(E const&));
} // namespace Private

/*
 * Adapter: binds a typed handler `void T::Method(E const&)` to the type-erased
 * `FunctionRef<void(void*)>` used by TEvent/EventRef.
 * Holds a single pointer to the consumer. Non-owning, non-movable:
 * the delegate() points to this object, so it must outlive the subscription.
 *
 *   EventHandler<Consumer, &Consumer::OnExact> handler{consumer};
 *   event_ref.subscribe(handler.delegate());
 */
template <class T, auto Method>
class EventHandler
{
public:
    using EventType = decltype(Private::event_arg_of(Method));

    explicit EventHandler(T& consumer) noexcept
        : m_consumer(&consumer)
    {
    }

    EventHandler(EventHandler const&) = delete;
    EventHandler& operator=(EventHandler const&) = delete;

    void invoke(void* raw)
    {
        (m_consumer->*Method)(*static_cast<EventType const*>(raw));
    }

    FunctionRef<void(void*)> delegate()
    {
        return ::ducta::makeFunctionRef<EventHandler, &EventHandler::invoke>(*this);
    }

private:
    T* m_consumer;
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_EVENT_HANDLER_HPP
