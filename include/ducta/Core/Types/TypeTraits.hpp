#ifndef DUCTA_CORE_TYPES_TYPETRAITS_HPP
#define DUCTA_CORE_TYPES_TYPETRAITS_HPP

#include <type_traits>

namespace ducta {

template<class T>
using remove_cv_t = typename std::remove_cv<T>::type;
template<class T>
using remove_reference_t = typename std::remove_reference<T>::type;


} // namespace ducta

#endif // DUCTA_CORE_TYPES_TYPETRAITS_HPP