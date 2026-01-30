#ifndef DUCTA_CORE_TYPES_SPAN_HPP
#define DUCTA_CORE_TYPES_SPAN_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/span.h>
#else
#include <span>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename T>
using Span = etl::span<T>;
#else
template <typename T>
using Span = std::span<T>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_SPAN_HPP