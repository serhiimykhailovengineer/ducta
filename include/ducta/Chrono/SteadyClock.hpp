#ifndef DUCTA_CHRONO_STEADYCLOCK_HPP
#define DUCTA_CHRONO_STEADYCLOCK_HPP

#include "ducta/Chrono/Chrono.hpp"

namespace ducta {
namespace Chrono {

class SteadyClock
{
public:
    using Timestamp = TimestampUS;

    /*
     * @brief Get the epoch timestamp of the steady clock (usually 0)
     * @return The epoch timestamp of the steady clock
    */
    Timestamp epoch() const;

    /*
     * @brief Get the current timestamp of the steady clock
     * @return The current timestamp of the steady clock
    */
    Timestamp now() const;

    /*
     * @brief Sleep for the specified duration using the steady clock
     * @param duration The duration to sleep
    */
    void sleepFor(Timestamp duration) const;
};

} // namespace Chrono
} // namespace ducta

#include "ducta/Chrono/Private/SteadyClock.inl"

#endif // DUCTA_CHRONO_STEADYCLOCK_HPP