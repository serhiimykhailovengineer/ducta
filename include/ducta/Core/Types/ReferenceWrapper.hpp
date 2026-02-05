#ifndef DUCTA_CORE_REFERENCE_WRAPPER_HPP
#define DUCTA_CORE_REFERENCE_WRAPPER_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/functional.h>

namespace ducta {
using etl::reference_wrapper;
using etl::ref;
using etl::cref;
} // namespace ducta

#else

#include <functional>

namespace ducta {
using std::reference_wrapper;
using std::ref;
using std::cref;
} // namespace ducta

#endif

#endif // DUCTA_CORE_REFERENCE_WRAPPER_HPP