#ifndef DUCTA_CORE_TYPES_ARRAY_HPP
#define DUCTA_CORE_TYPES_ARRAY_HPP

#include "ducta/Core/Types/Error.hpp"
#include "ducta/Core/Types/Expected.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/array.h>
#include "ducta/Core/ETLProfile.hpp"
#else
#include <array>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <class T, std::size_t N>
using Array = etl::array<T, N>;
#else

template <class T, std::size_t N>
using Array = std::array<T, N>;

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_ARRAY_HPP