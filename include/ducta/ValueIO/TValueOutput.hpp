#ifndef DUCTA_VALUEIO_TVALUE_OUTPUT_HPP
#define DUCTA_VALUEIO_TVALUE_OUTPUT_HPP

#include "ducta/Utils/TypeIndex.hpp"
#include "ducta/Utils/Deferred.hpp"
#include "ducta/ValueIO/Binder.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace ducta {
namespace IO {
    
template <typename T>
class TValueOutput
{
public:
    using value_type = T;

public:
    TValueOutput()
    : _value{}
    , _binder{[this]() -> T const& { return std::ref(_value); }}
    {
    }

    Binder<T>& get_binder() { return _binder; }

    Utils::TypeIndex type_id() const
    {
        return Utils::type_id<T>();
    }

    void set(T const& value) { _value = value; }
    void set(T&& value) { _value = std::move(value); }

private:
    T _value;
    Binder<T> _binder;
};

// template <typename T>
// Utils::Deferred bind(ValueOutput<T>& output, ValueInput<T>& input)
// {
//     auto& binder = output.get_binder();

//     return binder.bind(input);
// }

// template <typename T, typename... Inputs>
// std::vector<Utils::Deferred> bind(ValueOutput<T>& output, ValueInput<T>& input, Inputs&... inputs)
// {
//     return {bind(output, input), bind(output, inputs)...};
// }

template <typename T>
inline Utils::Deferred bind(TValueOutput<T>& output, InputRef input)
{
    if(auto value_input = cast<TValueInput<T>>(input))
    {
        auto& binder = output.get_binder();

        return binder.bind(*value_input);
    }
    throw std::runtime_error("Incompatible types for binding TValueOutput");

    return {};

}

} // namespace IO
} // namespace ducta

#endif // DUCTA_VALUEIO_TVALUE_OUTPUT_HPP