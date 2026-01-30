#ifndef DUCTA_CORE_TYPES_FUNCTION_HPP
#define DUCTA_CORE_TYPES_FUNCTION_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/function.h>
#else
#include <functional>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename Fn>
using Function = etl::function<Fn, 256>;
#else
template <typename Fn>
using Function = std::function<Fn>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_FUNCTION_HPP