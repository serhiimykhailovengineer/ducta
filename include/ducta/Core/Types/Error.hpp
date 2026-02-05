#ifndef DUCTA_CORE_TYPES_ERROR_HPP
#define DUCTA_CORE_TYPES_ERROR_HPP

#include "ducta/Core/Types/String.hpp"

namespace ducta {

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


} // namespace ducta

#endif // DUCTA_CORE_TYPES_ERROR_HPP