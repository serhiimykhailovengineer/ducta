#ifndef DUCTA_CORE_TYPES_DEFERRED_HPP
#define DUCTA_CORE_TYPES_DEFERRED_HPP

#include "ducta/Core/Types/FunctionRef.hpp"
#include "ducta/Core/Types/Optional.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {

template <class ValueType>
class Deferred
{
public:
    Deferred() = default;

    template <typename Func>
    Deferred(Func&& func, ValueType value)
        : _func(::ducta::forward<Func>(func))
        , _value(::ducta::move(value))
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
        : _func(::ducta::move(other._func))
        , _value(::ducta::move(other._value))
    {
        other.reset();
    }

    /* Runs the pending function first (like unique_ptr::reset), then takes over other's */
    Deferred& operator=(Deferred&& other) noexcept
    {
        if (this != &other)
        {
            if (_func)
            {
                _func(_value.value());
            }
            _func = ::ducta::move(other._func);
            _value = ::ducta::move(other._value);
            other.reset();
        }
        return *this;
    }

    /* false for a default constructed (or moved from) Deferred: nothing to run */
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(_func);
    }

private:
    void reset() noexcept
    {
        _func = {};
        _value = {};
    }

private:
    FunctionRef<void(ValueType const&)> _func;
    Optional<ValueType> _value;
};

template <class ValueType>
Deferred<ValueType> makeDeferred(FunctionRef<void(ValueType const&)> func, ValueType value)
{
    return Deferred<ValueType>{func, ::ducta::move(value)};
}

} // namespace ducta

#endif // DUCTA_CORE_TYPES_DEFERRED_HPP