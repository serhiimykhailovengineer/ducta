#ifndef DUCTA_CORE_UTILITY_HPP
#define DUCTA_CORE_UTILITY_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/utility.h>
#include "ducta/Core/ETLProfile.hpp"
#else
#include <utility>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

using etl::make_pair;
using etl::pair;

using etl::forward;
using etl::move;
using etl::swap;

using etl::declval;

using etl::in_place;
using etl::in_place_index_t;
using etl::in_place_t;
using etl::in_place_type_t;

#else

using std::make_pair;
using std::pair;

using std::forward;
using std::move;
using std::swap;

using std::declval;

using std::in_place;
using std::in_place_index_t;
using std::in_place_t;
using std::in_place_type_t;

#endif

} // namespace ducta

#endif // DUCTA_CORE_UTILITY_HPP