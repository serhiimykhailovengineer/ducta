#ifndef DUCTA_CORE_THREADS_INL
#define DUCTA_CORE_THREADS_INL

#include "ducta/Core/Threads.hpp"

#if defined(DUCTA_USE_EMBEDDED_BACKEND) && defined(DUCTA_USE_HOST_PLATFORM)
#include <chrono>
#include <thread>

#include "ducta/Core/Config.hpp"

// Host implementations of the platform hooks required by the embedded (ETL) backend.
// Opt-in via DUCTA_USE_HOST_PLATFORM, e.g. for running embedded builds on a desktop.

extern "C" DUCTA_DECL ETL_CHRONO_STEADY_CLOCK_DURATION::rep etl_get_steady_clock()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<ETL_CHRONO_STEADY_CLOCK_DURATION::rep>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

extern "C" DUCTA_DECL void ducta_sleep_for_us(etl::chrono::microseconds::rep duration)
{
    std::this_thread::sleep_for(std::chrono::microseconds(duration));
}
#endif

#endif // DUCTA_CORE_THREADS_INL
