#ifndef DUCTA_CORE_REFERENCE_WRAPPER_HPP
#define DUCTA_CORE_REFERENCE_WRAPPER_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/functional.h>
#include "ducta/Core/ETLProfile.hpp"

namespace ducta {
using etl::cref;
using etl::ref;
using etl::reference_wrapper;
} // namespace ducta

#else

#include <functional>

namespace ducta {
using std::cref;
using std::ref;
using std::reference_wrapper;
} // namespace ducta

#endif

#endif // DUCTA_CORE_REFERENCE_WRAPPER_HPP