#ifndef DUCTA_VALUEIO_BINDER_HPP
#define DUCTA_VALUEIO_BINDER_HPP

#include "ducta/Utils/TypeIndex.hpp"
#include "ducta/Utils/Deferred.hpp"
#include <functional>

namespace ducta {
namespace IO {

template <typename T>
class Binder
{
public:
    Binder(std::function<T const&()> getter)
    : _bind_func{std::move(getter)}
    {}

    template <typename InputT>
    Utils::Deferred bind(InputT& input)
    {
        input.notify(_bind_func());

        return Utils::Deferred{[this, &input]() {
            this->unbind(input);
        }};
    }

    template <typename InputT>
    void unbind(InputT& input)
    {
        input.release();
    }

    std::function<T const&()> get() const
    {
        return _bind_func;
    }

private:
    std::function<T const&()> _bind_func;
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_VALUEIO_BINDER_HPP