#ifndef DUCTA_CORE_TYPES_DEFERRED_HPP
#define DUCTA_CORE_TYPES_DEFERRED_HPP

#include "ducta/Core/Types/FunctionRef.hpp"
#include "ducta/Core/Types/Optional.hpp"

namespace ducta {

template <class ValueType>
class Deferred
{
public:
    Deferred() = default;

    template <typename Func>
    Deferred(Func&& func, ValueType value)
    : _func(std::forward<Func>(func))
    , _value(std::move(value))
    {
    }

    ~Deferred() noexcept
    {
        if (_func)
        {
            _func(_value.value());
        }
    }
    
    Deferred(Deferred const&) = delete;
    Deferred& operator=(Deferred const&) = delete;

    Deferred(Deferred&& other) noexcept
    : _func(std::move(other._func))
    , _value(std::move(other._value))
    {
        other.reset();
    }

    Deferred& operator=(Deferred&& other) noexcept
    {
        if (this != &other)
        {
            _func = std::move(other._func);
            _value = std::move(other._value);
            other.reset();
        }
        return *this;
    }

private:
    void reset() noexcept
    {
        _func  = {};
        _value = {};
    }

private:
    FunctionRef<void(ValueType const&)> _func;
    Optional<ValueType> _value;
};

template <class ValueType>
Deferred<ValueType> makeDeferred(FunctionRef<void(ValueType const&)> func, ValueType value)
{
    return Deferred<ValueType>{func, std::move(value)};
}

} // namespace ducta


#endif // DUCTA_CORE_TYPES_DEFERRED_HPP