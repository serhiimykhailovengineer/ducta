#ifndef DUCTA_CHRONO_CHRONO_INL
#define DUCTA_CHRONO_CHRONO_INL

#include "ducta/Chrono/Chrono.hpp"

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
#include <thread>
#else
#include "etl/thread.h"
#endif

namespace ducta {
namespace Chrono {

TimestampUS getSteadyTimestampUS()
{
    auto now = chrono::steady_clock::now();
    
    auto timestamp = chrono::duration_cast<TimestampUS>(now.time_since_epoch());
    return timestamp;
}

void sleepFor(TimestampUS duration)
{
    this_thread::sleep_for(duration);
}
} // namespace Chrono
} // namespace ducta


#endif // DUCTA_CHRONO_CHRONO_INL