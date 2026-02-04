#ifndef DUCTA_CORE_ALGORITHM_FORMAT_HPP
#define DUCTA_CORE_ALGORITHM_FORMAT_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/format.h>
#else
#include <fmt/format.h>

#include <utility>
#include <iterator>
#endif

#include "ducta/Core/Utility.hpp"

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename... Args>
using format_string = etl::format_string<Args...>;

template <class StringType, class... Args>
StringType& format_to(StringType& s, format_string<Args...> fmt, Args&&... args)
{
    etl::format_to(s, fmt, forward<Args>(args)...);
    return s;
}

#else

template <typename... Args>
using format_string = fmt::format_string<Args...>;

template <class StringType, class... Args>
StringType& format_to(StringType& s, format_string<Args...> fmt, Args&&... args)
{
    fmt::format_to(std::back_inserter(s), fmt, std::forward<Args>(args)...);
    return s;
}
#endif

} // namespace ducta

#endif // DUCTA_CORE_ALGORITHM_FORMAT_HPP