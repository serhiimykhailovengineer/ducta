#ifndef DUCTA_CORE_LIMITS_HPP
#define DUCTA_CORE_LIMITS_HPP

#include "ducta/Core/Config.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/limits.h>
#include "ducta/Core/ETLProfile.hpp"
#else
#include <limits>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using etl::numeric_limits;
#else
using std::numeric_limits;
#endif

} // namespace ducta

#endif // DUCTA_CORE_LIMITS_HPP
