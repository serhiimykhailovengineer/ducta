#ifndef DUCTA_UTILS_TYPEINDEX_HPP
#define DUCTA_UTILS_TYPEINDEX_HPP

#include <boost/type_index.hpp>

namespace ducta {
namespace Utils {

using TypeIndex = boost::typeindex::type_index;

template <typename T>
TypeIndex type_id()
{
    return boost::typeindex::type_id<T>();
}

} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_TYPEINDEX_HPP

