#ifndef DUCTA_CORE_TYPES_OPTIONAL_HPP
#define DUCTA_CORE_TYPES_OPTIONAL_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <etl/optional.h>
#else
#include <boost/optional.hpp>
#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
template <typename T>
using Optional = etl::optional<T>;

template <typename T>
class OptionalRef
{
public:
    OptionalRef() = default;
    OptionalRef(T const& value)
        : _ptr(&value)
    {
    }

    bool has_value() const
    {
        return _ptr != nullptr;
    }

    explicit operator bool() const
    {
        return has_value();
    }

    T const& value() const
    {
        // if (!has_value())
        // {
        //     throw etl::optional_exception(ETL_ERROR_TEXT("optional:bad_access", "DUCTA_CORE_TYPES_OPTIONAL_HPP"), ETL_FILE, ETL_LINE);
        // }
        return *_ptr;
    }

    T const& value_or(T const& default_value) const
    {
        return has_value() ? *_ptr : default_value;
    }

    void reset()
    {
        _ptr = nullptr;
    }

    void emplace(T const& value)
    {
        _ptr = &value;
    }

private:
    T const* _ptr = nullptr;
};

#else
template <typename T>
using Optional = boost::optional<T>;

template <typename T>
using OptionalRef = boost::optional<T const&>;

#endif

} // namespace ducta

#endif // DUCTA_CORE_TYPES_OPTIONAL_HPP