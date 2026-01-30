#ifndef DUCTA_T_OUTPUT_INL
#define DUCTA_T_OUTPUT_INL

#include "ducta/IO/TInput.hpp"
#include "ducta/IO/InputRef.hpp"

#include "ducta/Core/Types/TypeIndex.hpp"
#include "ducta/Core/Types/Deferred.hpp"

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
    _value = std::move(value);
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
    set(std::move(value));
    return *this;
}

template <typename T>
Deferred TOutput<T>::bind(InputRef const& input)
{
    if (input.compatible(type_id<T>()))
    {
        _inputs.emplace_back(input);
        return Deferred{[this, input = std::move(input)]() {
            // Remove input from the list upon destruction
            _inputs.erase(std::remove_if(_inputs.begin(), _inputs.end(),
                [&input](InputRef& ref) { return ref == input; }),
                _inputs.end());
        }};
    }

    return {};
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_OUTPUT_INL