#ifndef DUCTA_CORE_TYPES_DEFERRED_HPP
#define DUCTA_CORE_TYPES_DEFERRED_HPP

#include "ducta/Core/Types/Function.hpp"

namespace ducta {

class Deferred
{
public:
    Deferred() = default;

    template <typename Func>
    Deferred(Func&& func)
    : _func(makeFunction(std::forward<Func>(func)))
    {}

    ~Deferred()
    {
        if (_func)
        {
            _func();
        }
    }
    
    Deferred(Deferred const&) = delete;
    Deferred& operator=(Deferred const&) = delete;

    Deferred(Deferred&& other) noexcept
    : _func(std::move(other._func))
    {
        other = {};
    }

    Deferred& operator=(Deferred&& other) noexcept
    {
        if (this != &other)
        {
            _func = std::move(other._func);
            other = {};
        }
        return *this;
    }


private:
    Function<void()> _func;
};

} // namespace ducta


#endif // DUCTA_CORE_TYPES_DEFERRED_HPP