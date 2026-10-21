#ifndef DUCTA_CORE_THREADS_HPP
#define DUCTA_CORE_THREADS_HPP

#include "ducta/Core/Config.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <thread>
#else
#include "ducta/Core/Private/EtlChrono.hpp"
#endif

#if defined(DUCTA_USE_EMBEDDED_BACKEND)
/**
 * @brief Platform hook: sleep for the specified duration in microseconds
 * @note Must be provided by the target platform, like etl_get_steady_clock()
 */
extern "C" void ducta_sleep_for_us(etl::chrono::microseconds::rep duration);
#endif

namespace ducta {

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
namespace this_thread = std::this_thread;
#else
namespace this_thread {

/**
 * @brief Sleep for at least the specified duration
 * @param duration The duration to sleep
 */
template <typename Rep, typename Period>
void sleep_for(etl::chrono::duration<Rep, Period> const& duration)
{
    auto duration_us = etl::chrono::duration_cast<etl::chrono::microseconds>(duration);
    ducta_sleep_for_us(duration_us.count());
}

} // namespace this_thread
#endif

} // namespace ducta

#include "ducta/Core/Private/Threads.inl"

#endif // DUCTA_CORE_THREADS_HPP
