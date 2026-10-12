#ifndef DUCTA_CORE_TYPES_ARRAY_HPP
#define DUCTA_CORE_TYPES_ARRAY_HPP

#include "ducta/Core/Types/Expected.hpp"
#include "ducta/Core/Types/Error.hpp"


#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/array.h>
#else
#include <array>
#endif


namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template<class T, std::size_t N> 
using Array = etl::array<T, N>;
#else

template<class T, std::size_t N> 
using Array = std::array<T, N>;

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_ARRAY_HPP