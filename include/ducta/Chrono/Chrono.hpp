#ifndef DUCTA_CHRONO_CHRONO_HPP
#define DUCTA_CHRONO_CHRONO_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Threads.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <chrono>
#else
#include "ducta/Core/Private/EtlChrono.hpp"
#endif

namespace ducta {
namespace Chrono {

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
namespace chrono = std::chrono;

using TimestampUS = chrono::microseconds;
#else
namespace chrono = etl::chrono;

using TimestampUS = chrono::microseconds;
#endif

/**
 * @brief Get the current steady timestamp in microseconds
 * @return The current steady timestamp in microseconds
 */
inline TimestampUS getSteadyTimestampUS();

/**
 * @brief Sleep for the specified duration in microseconds
 * @param duration The duration to sleep
 */
inline void sleepFor(TimestampUS duration);

} // namespace Chrono
} // namespace ducta

#include "ducta/Chrono/Private/Chrono.inl"

#endif // DUCTA_CHRONO_CHRONO_HPP