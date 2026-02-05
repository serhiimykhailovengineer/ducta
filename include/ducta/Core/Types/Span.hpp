#ifndef DUCTA_CORE_TYPES_SPAN_HPP
#define DUCTA_CORE_TYPES_SPAN_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/span.h>
#else
#include <boost/core/span.hpp>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename T>
using Span = etl::span<T>;
#else
template <typename T>
using Span = boost::span<T>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_SPAN_HPP