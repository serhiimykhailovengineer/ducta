#ifndef DUCTA_CORE_TYPES_FUNCTION_REF_HPP
#define DUCTA_CORE_TYPES_FUNCTION_REF_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/delegate.h>
#else
#include <functional>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename Fn>
using FunctionRef = etl::delegate<Fn>;

template <typename TLambda>
constexpr auto makeFunctionRef(TLambda& instance) noexcept
{
    return etl::make_delegate<TLambda>(instance);
}

template <typename T, auto Method>
constexpr auto makeFunctionRef(T& instance) noexcept
{
    return etl::make_delegate<T, Method>(instance);
}

#else
template <typename Fn>
using FunctionRef = std::function<Fn>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_FUNCTION_REF_HPP