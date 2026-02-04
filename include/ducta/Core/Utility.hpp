#ifndef DUCTA_CORE_UTILITY_HPP
#define DUCTA_CORE_UTILITY_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/utility.h>
#else
#include <utility>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

using etl::swap;
using etl::forward;
using etl::move;

using etl::declval;

#else

using std::swap;
using std::forward;
using std::move;

using std::declval;

#endif

} // namespace ducta

#endif // DUCTA_CORE_UTILITY_HPP