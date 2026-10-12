#ifndef DUCTA_IO_TEVENT_HPP
#define DUCTA_IO_TEVENT_HPP

#include "ducta/Core/TypeTraits.hpp"
#include "ducta/Core/Types/FunctionRef.hpp"

namespace ducta {
namespace IO {

/*
 * TEvent class template for emitting events of type T.
 */
template <class T>
class TEvent
{
public:
    using EventType = ::ducta::decay_t<T>;
    using EventDelegate = FunctionRef<void(void*)>;

public:
    TEvent() = default;

    /*
     * @brief Emit an event of type EventType to the subscribed nodes
     * @tparam EventType The type of the event
     * @param event The event to emit
     */
    void operator()(EventType const& event) const;

    /*
     * @brief Subscribe to the event
     * @param delegate The delegate to call when the event is emitted
     */
    void subscribe(EventDelegate delegate);

    /*
     * @brief Unsubscribe from the event
     */
    void unsubscribe();

private:
    EventDelegate m_delegate;
};

} // namespace IO
} // namespace ducta

#include "ducta/IO/Private/TEvent.inl"

#endif // DUCTA_IO_TEVENT_HPP