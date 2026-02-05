#ifndef DUCTA_T_OUTPUT_INL
#define DUCTA_T_OUTPUT_INL

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/InputRef.hpp"
#include "ducta/IO/Connection.hpp"

#include "ducta/Core/Types/TypeIndex.hpp"
#include "ducta/Core/Algorithms/Adapters.hpp"

namespace ducta {
namespace IO {

template <typename T>
TOutput<T>::TOutput()
: _value{}
{
}

template <typename T>
void TOutput<T>::set(value_type const& value) 
{
    _value = value;
    for (auto& input_ref : _inputs)
    {
        input_ref.notify(_value);
    }
}

template <typename T>
void TOutput<T>::set(value_type&& value) 
{
    _value = ::ducta::move(value);
    for (auto& input_ref : _inputs)
    {
        input_ref.notify(_value);
    }
}

template <typename T>
TOutput<T>& TOutput<T>::operator=(value_type const& value)
{
    set(value);
    return *this;
}

template <typename T>
TOutput<T>& TOutput<T>::operator=(value_type&& value)
{
    set(::ducta::move(value));
    return *this;
}

template <typename T>
Connection TOutput<T>::bind(InputRef const& input)
{
    if (input.compatible(type_id<T>()))
    {
        _inputs.emplace_back(input);

        return makeConnection(makeFunctionRef<TOutput<T>, &TOutput<T>::unbind>(*this), input);
    }

    return {};
}

template <typename T>
void TOutput<T>::unbind(InputRef const& input)
{
    ::ducta::erase(_inputs, input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_INL