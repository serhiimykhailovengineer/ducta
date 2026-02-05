#ifndef DUCTA_CORE_TYPES_STRINGVIEW_HPP
#define DUCTA_CORE_TYPES_STRINGVIEW_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/string_view.h>
#else
#include <string_view>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using StringView = etl::string_view;
#else
using StringView = std::string_view;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_STRINGVIEW_HPP