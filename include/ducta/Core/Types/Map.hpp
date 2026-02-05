#ifndef DUCTA_CORE_TYPES_MAP_HPP
#define DUCTA_CORE_TYPES_MAP_HPP

#include "ducta/Core/Utility.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/map.h>
#else
#include <map>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename K, typename V, const size_t SIZE, typename KeyCompare = etl::less<K>>
using Map = etl::map<K, V, SIZE, KeyCompare>;

namespace detail {

// Detect etl::pair<*, *>
template <class T>
struct is_etl_pair : std::false_type {};

template <class A, class B>
struct is_etl_pair<etl::pair<A, B>> : std::true_type {};

template <class... Args>
using all_etl_pairs = std::conjunction<is_etl_pair<std::decay_t<Args>>...>;

// Insert K,V,... (two-at-a-time)
template <typename K, typename V, size_t SIZE>
inline void map_emplace_kv(etl::map<K, V, SIZE>&) noexcept
{
    // Base case: nothing to insert
}

template <typename K, typename V, size_t SIZE, class K1, class V1, class... Rest>
inline void map_emplace_kv(etl::map<K, V, SIZE>& map, K1&& k, V1&& v, Rest&&... rest) noexcept
{
    // Construct value_type and forward to insert
    map.insert(typename etl::map<K, V, SIZE>::value_type{ std::forward<K1>(k), std::forward<V1>(v) });

    if constexpr (sizeof...(Rest) > 0)
    {
        map_emplace_kv<K, V, SIZE>(map, std::forward<Rest>(rest)...);
    }
}

} // namespace detail

template <typename K, typename V, size_t SIZE, class... Args>
inline void map_emplace(etl::map<K, V, SIZE>& map, Args&&... args) noexcept
{
    if constexpr (sizeof...(Args) == 0)
    {
        return;
    }
    else if constexpr (detail::all_etl_pairs<Args...>::value)
    {
        // All args are etl::pair<*,*> (forward rvalues as rvalues, lvalues as lvalues)
        (map.insert(std::forward<Args>(args)), ...);
    }
    else
    {
        static_assert(sizeof...(Args) % 2 == 0, "map_emplace(K,V,...) requires an even number of arguments");
        detail::map_emplace_kv<K, V, SIZE>(map, std::forward<Args>(args)...);
    }
}

template <typename K, typename V, const size_t SIZE, class... Args>
auto makeMap(Args&&... args) noexcept
{
    etl::map<K, V, SIZE> result;
    map_emplace<K, V, SIZE>(result, ::ducta::forward<Args>(args)...);
    return result;
}

#else
template <typename K, typename V, const size_t SIZE>
using Map = std::map<K, V>;

template <typename K, typename V, const size_t SIZE, class... Args>
auto makeMap(Args&&... args) noexcept
{
    return std::map<K, V>{std::forward<Args>(args)...};
}
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_MAP_HPP