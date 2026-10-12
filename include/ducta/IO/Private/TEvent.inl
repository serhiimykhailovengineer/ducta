#ifndef DUCTA_T_EVENT_INL
#define DUCTA_T_EVENT_INL

#include "ducta/IO/TEvent.hpp"

namespace ducta {
namespace IO {

template<class T>
void TEvent<T>::operator()(EventType const& event) const
{
    if (m_delegate)
    {
        m_delegate(const_cast<void*>(static_cast<void const*>(&event)));
    }
}

template<class T>
void TEvent<T>::subscribe(EventDelegate delegate)
{
    m_delegate = delegate;
}

template<class T>
void TEvent<T>::unsubscribe()
{
    m_delegate = EventDelegate{};
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_EVENT_INL