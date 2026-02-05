#ifndef DUCTA_CORE_UTILITY_HPP
#define DUCTA_CORE_UTILITY_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/utility.h>
#else
#include <utility>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

using etl::pair;
using etl::make_pair;

using etl::swap;
using etl::forward;
using etl::move;

using etl::declval;

using etl::in_place_t;
using etl::in_place_type_t;
using etl::in_place_index_t;
using etl::in_place;

#else

using std::pair;
using std::make_pair;

using std::swap;
using std::forward;
using std::move;

using std::declval;

using std::in_place_t;
using std::in_place_type_t;
using std::in_place_index_t;
using std::in_place;

#endif

} // namespace ducta

#endif // DUCTA_CORE_UTILITY_HPP