#ifndef DUCTA_CORE_TYPES_EXPECTED_HPP
#define DUCTA_CORE_TYPES_EXPECTED_HPP

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

#include <etl/expected.h>

#else

#include <boost/system/result.hpp>

#include <utility>

#endif

namespace ducta {

#if defined(DUCTA_USE_EMBEDDED_BACKEND)

template<class Value, class Error>
using Expected = etl::expected<Value, Error>;

#else

template<class Value, class Error>
using Expected = boost::system::result<Value, Error>;

template <class Error>
struct Unexpected
{
    Error error;

    // Convert to Expected<Value,Error> for any Value.
    template <class Value>
    operator Expected<Value, Error>() const &
    {
        return {boost::system::in_place_error, error};
    }

    template <class Value>
    operator Expected<Value, Error>() &&
    {
        return {boost::system::in_place_error, std::move(error)};
    }
};

#endif



} // namespace ducta

#endif // DUCTA_CORE_TYPES_EXPECTED_HPP