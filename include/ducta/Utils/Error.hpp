#ifndef DUCTA_UTILS_ERROR_HPP
#define DUCTA_UTILS_ERROR_HPP

#include "ducta/Utils/String.hpp"

namespace ducta {
namespace Utils {

struct Error
{
    String<100> message;
};

template <class OutputStream>
OutputStream& operator<<(OutputStream& os, const Error& error)
{
    os << error.message;
    return os;
}


} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_ERROR_HPP