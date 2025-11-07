#ifndef DUCTA_T_INPUT_INL
#define DUCTA_T_INPUT_INL

#include "ducta/io/TInput.hpp"
#include "ducta/io/Private/TInputModel.hpp"

namespace ducta {
namespace IO {

template <typename T>
template <typename TInputType>
TInput<T>::TInput(TInputType&& input)
: m_input(std::make_unique<Private::TInputModel<std::decay_t<TInputType>, T>>(std::forward<TInputType>(input)))
{
}

template <typename T>
bool TInput<T>::ready() const
{
    return m_input->is_ready();
}

template <typename T>
T const& TInput<T>::value() const
{
    return m_input->get_value();
}

template <typename T>
T const& TInput<T>::value_or(T const& default_value) const
{
    if (ready())
    {
        return value();
    }
    return default_value;
}

template <typename T>
void TInput<T>::notify(T const& value)
{
    m_input->do_notify(value);
}

template <typename T>
void TInput<T>::set_callback(std::function<void(T const&)> callback)
{
    m_input->set_callback(std::move(callback));
}

template <typename T>
InputRef TInput<T>::get_ref()
{
    return m_input->get_ref();
}

template <typename T>
TInput<T>::operator T const&() const
{
    return value();
}

template <typename T>
TInput<T>::operator bool() const
{
    return ready();
}

} // namespace io
} // namespace ducta

#endif // DUCTA_T_INPUT_INL