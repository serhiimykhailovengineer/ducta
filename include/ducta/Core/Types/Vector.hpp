#ifndef DUCTA_CORE_TYPES_VECTOR_HPP
#define DUCTA_CORE_TYPES_VECTOR_HPP


#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/vector.h>
#else
#include <vector>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename T, const size_t MAX_SIZE>
using Vector = etl::vector<T, MAX_SIZE>;
#else
template <typename T, const size_t MAX_SIZE = 0>
using Vector = std::vector<T>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_VECTOR_HPP