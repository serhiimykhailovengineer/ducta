#ifndef DUCTA_CORE_TYPES_VECTOR_HPP
#define DUCTA_CORE_TYPES_VECTOR_HPP


#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include "ducta/Core/ETLProfile.hpp"
#include <etl/vector.h>
#else
#include <vector>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename T, const size_t MAX_SIZE>
using Vector = etl::vector<T, MAX_SIZE>;

template <typename T, class... Args>
Vector<T, sizeof...(Args)> makeVector(Args&&... args)
{
    Vector<T, sizeof...(Args)> vec;
    (vec.push_back(::ducta::forward<Args>(args)), ...);
    return vec;
}

template< class InputIt, class T >
InputIt find(InputIt first, InputIt last, T const& value )
{
    for (; first != last; ++first)
    {
        if (*first == value)
        {
            return first;
        }
    }
    return last;
}

template <typename T, const size_t MAX_SIZE>
bool erase(Vector<T, MAX_SIZE>& vec, const T& value)
{
    auto it = ::ducta::find(vec.begin(), vec.end(), value);
    if (it != vec.end())
    {
        vec.erase(it);
        return true;
    }
    return false;
}

#else
template <typename T, const size_t MAX_SIZE = 0>
using Vector = std::vector<T>;

template <typename T, class... Args>
Vector<T> makeVector(Args&&... args)
{
    return Vector<T>{::ducta::forward<Args>(args)...};
}

template <typename T, const size_t MAX_SIZE = 0>
bool erase(Vector<T, MAX_SIZE>& vec, const T& value)
{
    auto it = std::remove(vec.begin(), vec.end(), value);
    if (it != vec.end())
    {
        vec.erase(it);
        return true;
    }
    return false;
}

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_VECTOR_HPP