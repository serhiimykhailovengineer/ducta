#ifndef DUCTA_CORE_TYPES_TYPEINDEX_HPP
#define DUCTA_CORE_TYPES_TYPEINDEX_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/type_index.h>
#else
#include <boost/type_index.hpp>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
using TypeIndex = etl::type_index;
template <typename T>
TypeIndex type_id()
{
    return etl::type_id<T>();
}
#else
using TypeIndex = boost::typeindex::type_index;

template <typename T>
TypeIndex type_id()
{
    return boost::typeindex::type_id<T>();
}
#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_TYPEINDEX_HPP

