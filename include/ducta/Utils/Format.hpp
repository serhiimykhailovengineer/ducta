#ifndef DUCTA_UTILS_FORMAT_HPP
#define DUCTA_UTILS_FORMAT_HPP

#include <fmt/base.h>

namespace ducta {

template <typename... Args>
using format_string = fmt::format_string<Args...>;

template <class StringType, class... Args>
StringType& format_to(StringType& s, format_string<Args...> fmt, Args&&... args)
{
    fmt::format_to(std::back_inserter(s), fmt, std::forward<Args>(args)...);
    return s;
}

} // namespace ducta

#endif // DUCTA_UTILS_FORMAT_HPP