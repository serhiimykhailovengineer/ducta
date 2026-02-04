#ifndef DUCTA_CORE_TYPETRAITS_HPP
#define DUCTA_CORE_TYPETRAITS_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

#include <etl/type_traits.h>

#else

#include <type_traits>
#include <utility>

#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

using true_type = etl::true_type;
using false_type = etl::false_type;

using etl::decay;

template< class T >
using decay_t = typename decay<T>::type;

using etl::is_same;
using etl::is_same_v;

using etl::is_reference;

using etl::conditional_t;

template<class T>
using remove_cv_t = typename etl::remove_cv<T>::type;

template<class T>
using remove_reference_t = typename etl::remove_reference<T>::type;

#else

using true_type = std::true_type;
using false_type = std::false_type;

using std::is_same;
using std::is_same_v;

using std::decay;

using std::decay_t;

using std::is_reference;

using std::conditional_t;

template<class T>
using remove_cv_t = typename std::remove_cv<T>::type;
template<class T>
using remove_reference_t = typename std::remove_reference<T>::type;

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPETRAITS_HPP