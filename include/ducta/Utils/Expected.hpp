#ifndef DUCTA_UTILS_EXPECTED_HPP
#define DUCTA_UTILS_EXPECTED_HPP

#include <boost/system/result.hpp>

#include <utility>

namespace ducta {
namespace Utils {

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

} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_EXPECTED_HPP