/**
 * @file Connection.hpp
 * @brief Class that represents a RAII-connection.
 */
#ifndef DUCTA_CONNECTION_HPP
#define DUCTA_CONNECTION_HPP

#include "ducta/Core/Types/Deferred.hpp"

#include "ducta/IO/InputRef.hpp"

namespace ducta {
namespace IO {

using Connection = Deferred<InputRef>;

inline Connection makeConnection(FunctionRef<void(InputRef const&)> unbind_func, InputRef input)
{
    return makeDeferred<InputRef>(std::move(unbind_func), input);
}

} // namespace IO
} // namespace ducta

#endif // DUCTA_CONNECTION_HPP