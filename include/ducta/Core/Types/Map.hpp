#ifndef DUCTA_CORE_TYPES_MAP_HPP
#define DUCTA_CORE_TYPES_MAP_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/map.h>
#else
#include <map>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename K, typename V, const size_t SIZE, typename KeyCompare = etl::less<K>>
using Map = etl::map<K, V, SIZE, KeyCompare>;
#else
template <typename K, typename V, const size_t SIZE, typename KeyCompare = std::less<K>>
using Map = std::map<K, V, KeyCompare>;
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_MAP_HPP