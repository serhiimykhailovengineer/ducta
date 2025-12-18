#ifndef DUCTA_UTILS_FUNCTION_HPP
#define DUCTA_UTILS_FUNCTION_HPP

#include <functional>

namespace ducta {
namespace Utils {

template <typename Fn>
using Function = std::function<Fn>;

} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_FUNCTION_HPP