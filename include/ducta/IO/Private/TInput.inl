/**
 * @file TInput.inl
 * @brief Implementation of TInput template class.
 */

#ifndef DUCTA_T_INPUT_INL
#define DUCTA_T_INPUT_INL

#include "ducta/IO/TInput.hpp"

namespace ducta {
namespace IO {

template <typename T>
bool TInput<T>::compatible(TypeIndex type) const
{
    return type == ::ducta::type_id<value_type>();
}

template <typename T>
bool TInput<T>::ready() const
{
    return _value.has_value();
}

template <typename T>
typename TInput<T>::storage_type TInput<T>::value()
{
    if (!ready())
    {
        throw std::runtime_error("Input not ready");
    }

    storage_type result_value = _value.value();
    _value.reset();
    return result_value;
}

template <typename T>
typename TInput<T>::storage_type TInput<T>::value_or(value_type const& default_value)
{
    storage_type result_value = _value.value_or(default_value);
    _value.reset();
    return result_value;
}

template <typename T>
void TInput<T>::notify(T const& value)
{
    _value.emplace(value);
}

template <typename T>
void TInput<T>::release()
{
    _value.reset();
}

template <typename T>
TInput<T>::operator bool() const
{
    return ready();
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_INL