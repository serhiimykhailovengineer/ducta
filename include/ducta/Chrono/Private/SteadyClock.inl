#ifndef DUCTA_CHRONO_STEADYCLOCK_INL
#define DUCTA_CHRONO_STEADYCLOCK_INL

#include "ducta/Chrono/SteadyClock.hpp"

namespace ducta {
namespace Chrono {

SteadyClock::Timestamp SteadyClock::epoch() const
{
    return Timestamp{0};
}

SteadyClock::Timestamp SteadyClock::now() const
{
    return getSteadyTimestampUS();
}

void SteadyClock::sleepFor(Timestamp duration) const
{
    Chrono::sleepFor(duration);
}

} // namespace Chrono
} // namespace ducta

#endif // DUCTA_CHRONO_STEADYCLOCK_INL