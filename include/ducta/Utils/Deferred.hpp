#ifndef DUCTA_UTILS_DEFERRED_HPP
#define DUCTA_UTILS_DEFERRED_HPP

#include <functional>

namespace ducta {
namespace Utils {

class Deferred
{
public:
    Deferred() = default;

    template <typename Func>
    Deferred(Func&& func)
    : _func(std::forward<Func>(func))
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
        other._func = nullptr;
    }

    Deferred& operator=(Deferred&& other) noexcept
    {
        if (this != &other)
        {
            _func = std::move(other._func);
            other._func = nullptr;
        }
        return *this;
    }


private:
    std::function<void()> _func;
};

} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_DEFERRED_HPP