// Platform hooks of the embedded backend: the time of etl::chrono::steady_clock and a sleep.
// With DUCTA_USE_HOST_PLATFORM (env:native) Ducta provides them for the computer.

#if defined(ARDUINO)

#include <Arduino.h>

#include <etl/chrono.h>

#include "ducta/Core/Threads.hpp"

/* Nanoseconds (the default ETL_CHRONO_STEADY_CLOCK_DURATION) since start-up; micros() wraps
 * after ~71 minutes, so the wraps are counted */
extern "C" ETL_CHRONO_STEADY_CLOCK_DURATION::rep etl_get_steady_clock()
{
    static uint32_t last = 0;
    static uint64_t wraps = 0;
    uint32_t const now = micros();
    if (now < last)
    {
        ++wraps;
    }
    last = now;
    uint64_t const us = (wraps << 32) | now;
    return static_cast<ETL_CHRONO_STEADY_CLOCK_DURATION::rep>(
        etl::chrono::duration_cast<ETL_CHRONO_STEADY_CLOCK_DURATION>(etl::chrono::microseconds(us))
            .count());
}

extern "C" void ducta_sleep_for_us(etl::chrono::microseconds::rep duration)
{
    if (duration >= 1000)
    {
        delay(static_cast<unsigned long>(duration / 1000));
    }
    delayMicroseconds(static_cast<unsigned int>(duration % 1000));
}

#endif
