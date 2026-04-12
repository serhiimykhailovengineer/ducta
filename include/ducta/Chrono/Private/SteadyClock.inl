#ifndef DUCTA_CHRONO_STEADYCLOCK_INL
#define DUCTA_CHRONO_STEADYCLOCK_INL

#include "ducta/Chrono/SteadyClock.hpp"

namespace ducta {
namespace Chrono {

inline SteadyClock::Timestamp SteadyClock::epoch() const
{
    return Timestamp{0};
}

inline SteadyClock::Timestamp SteadyClock::now() const
{
    return getSteadyTimestampUS();
}

inline void SteadyClock::sleepFor(Timestamp duration) const
{
    Chrono::sleepFor(duration);
}

} // namespace Chrono
} // namespace ducta

#endif // DUCTA_CHRONO_STEADYCLOCK_INL