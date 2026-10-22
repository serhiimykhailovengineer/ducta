#ifndef DUCTA_CORE_ALGORITHM_FORMAT_HPP
#define DUCTA_CORE_ALGORITHM_FORMAT_HPP

#include "ducta/Core/Config.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/format.h>
#include "ducta/Core/ETLProfile.hpp"
#else
#include <fmt/format.h>

#include <iterator>
#include <utility>
#endif

#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Utility.hpp"

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename... Args>
using format_string = etl::format_string<Args...>;

template <class StringType, class... Args>
StringType& format_to(StringType& s, format_string<Args...> fmt, Args&&... args)
{
    etl::format_to(s, fmt, ::ducta::forward<Args>(args)...);
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

/*
 * @brief Text of at most Capacity characters, formatted by format_to_n(): no heap with either
 *        backend, and a longer text is truncated
 */
template <size_t Capacity>
class FormatBuffer
{
public:
    static constexpr size_t capacity = Capacity;

    char const* data() const noexcept
    {
        return m_data;
    }

    size_t size() const noexcept
    {
        return m_size;
    }

    StringView view() const noexcept
    {
        return StringView{m_data, m_size};
    }

    void clear() noexcept
    {
        m_size = 0;
    }

    /* Replace the text (see format_to_n()) */
    template <class... Args>
    StringView format(format_string<Args...> fmt, Args&&... args)
    {
#if defined(DUCTA_USE_EMBEDDED_BACKEND)
        char* const end = etl::format_to_n(m_data, Capacity, fmt, ::ducta::forward<Args>(args)...);
#else
        char* const end = fmt::format_to_n(m_data, Capacity, fmt, std::forward<Args>(args)...).out;
#endif
        m_size = static_cast<size_t>(end - m_data);
        return view();
    }

private:
    char m_data[Capacity]{};
    size_t m_size{0};
};

/* Format into out, replacing its text: at most Capacity characters, the rest is dropped */
template <size_t Capacity, class... Args>
StringView format_to_n(FormatBuffer<Capacity>& out, format_string<Args...> fmt, Args&&... args)
{
    return out.format(fmt, ::ducta::forward<Args>(args)...);
}

} // namespace ducta

#endif // DUCTA_CORE_ALGORITHM_FORMAT_HPP