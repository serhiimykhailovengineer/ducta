#ifndef DUCTA_CORE_ALGORITHM_ADAPTERS_HPP
#define DUCTA_CORE_ALGORITHM_ADAPTERS_HPP

namespace ducta {
namespace Adapters {

template <typename T>
struct Reversed
{
    T& iterable;

    explicit Reversed(T& iterable)
        : iterable(iterable)
    {
    }

    auto begin() { return iterable.rbegin(); }
    auto end() { return iterable.rend(); }
};

} // namespace Adapters
} // namespace ducta


#endif // DUCTA_CORE_ALGORITHM_ADAPTERS_HPP