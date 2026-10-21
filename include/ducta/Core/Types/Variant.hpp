#ifndef DUCTA_CORE_TYPES_VARIANT_HPP
#define DUCTA_CORE_TYPES_VARIANT_HPP

#include "ducta/Core/Config.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/variant.h>
#include "ducta/Core/ETLProfile.hpp"
#else
#include <variant>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

template <class... Types>
using Variant = etl::variant<Types...>;

using etl::get;
using etl::get_if;
using etl::holds_alternative;

template <class Visitor, class... Variants>
constexpr decltype(auto) visit(Visitor&& v, Variants&&... values)
{
    return etl::visit(etl::forward<Visitor>(v), etl::forward<Variants>(values)...);
}

#else

template <class... Types>
using Variant = std::variant<Types...>;

using std::get;
using std::get_if;
using std::holds_alternative;

template <class Visitor, class... Variants>
constexpr decltype(auto) visit(Visitor&& v, Variants&&... values)
{
    return std::visit(std::forward<Visitor>(v), std::forward<Variants>(values)...);
}

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_VARIANT_HPP
