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

} // namespace IO
} // namespace ducta

#endif // DUCTA_CONNECTION_HPP