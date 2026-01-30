#ifndef DUCTA_UTILS_VARIANT_HPP
#define DUCTA_UTILS_VARIANT_HPP

namespace ducta {
namespace Utils {

template<class... Types >
using Variant = std::variant<Types...>;

template<class Visitor, class... Variants>
constexpr decltype(auto) visit(Visitor&& v, Variants&&... values)
{
    return std::visit(std::forward<Visitor>(v), std::forward<Variants>(values)...);
}

} // namespace Utils
} // namespace ducta
#endif // DUCTA_UTILS_VARIANT_HPP