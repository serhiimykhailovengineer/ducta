#ifndef DUCTA_CORE_TYPES_STRING_HPP
#define DUCTA_CORE_TYPES_STRING_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/string.h>
#else
#include <string>
#endif


namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <size_t MAX_SIZE>
using String = etl::string<MAX_SIZE>;
#else
template <size_t MAX_SIZE>
using String = std::string;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_STRING_HPP