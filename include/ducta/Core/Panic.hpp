/**
 * @file Panic.hpp
 * @brief Report a misuse the caller cannot recover from (e.g. reading an input that is not ready)
 *
 * With exceptions enabled it throws std::runtime_error, so a node's iterate() can still be
 * caught by the pipeline. Without exceptions (-fno-exceptions, as most firmware is built) it
 * calls DUCTA_PANIC_HANDLER(message) if the application defines it (e.g. to log and reset the
 * MCU), then std::abort().
 */

#ifndef DUCTA_CORE_PANIC_HPP
#define DUCTA_CORE_PANIC_HPP

// Must not include other Ducta headers: Capacity.hpp includes it

#include <cstdlib>

#if defined(__cpp_exceptions)
#include <stdexcept>
#endif

#if defined(DUCTA_PANIC_HANDLER)
/* void DUCTA_PANIC_HANDLER(char const* message); defined by the application */
void DUCTA_PANIC_HANDLER(char const* message);
#endif

namespace ducta {

/* Not named panic(): the ESP8266 Arduino core defines a panic() macro */
[[noreturn]] inline void fatal_error(char const* message)
{
#if defined(__cpp_exceptions)
    throw std::runtime_error(message);
#else
#if defined(DUCTA_PANIC_HANDLER)
    DUCTA_PANIC_HANDLER(message);
#else
    (void)message;
#endif
    std::abort();
#endif
}

} // namespace ducta

#endif // DUCTA_CORE_PANIC_HPP
