#ifndef DUCTA_CHRONO_CHRONO_INL
#define DUCTA_CHRONO_CHRONO_INL

#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Chrono {

inline TimestampUS getSteadyTimestampUS()
{
    auto now = chrono::steady_clock::now();
    
    auto timestamp = chrono::duration_cast<TimestampUS>(now.time_since_epoch());
    return timestamp;
}

inline void sleepFor(TimestampUS duration)
{
    this_thread::sleep_for(duration);
}
} // namespace Chrono
} // namespace ducta


#endif // DUCTA_CHRONO_CHRONO_INL