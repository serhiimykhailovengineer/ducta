/**
 * @file EtlChrono.hpp
 * @brief <etl/chrono.h>, protected from macros of some Arduino cores. Ducta includes ETL's chrono
 *        only through this header; include it before <etl/chrono.h> in a file that also includes
 *        Arduino.h.
 *
 *  - abs(x): a function-like macro of e.g. the Renesas (Arduino UNO R4) core; ETL declares a
 *    function template abs(duration<Rep, Period>), which the macro breaks.
 *  - assert(e): the ESP8266 core's assert keeps a static string, which is not allowed in the
 *    constexpr functions of ETL's calendar types: ETL's chrono is compiled with NDEBUG there,
 *    then <assert.h> restores the application's assert.
 */

#ifndef DUCTA_CORE_PRIVATE_ETL_CHRONO_HPP
#define DUCTA_CORE_PRIVATE_ETL_CHRONO_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/ETLProfile.hpp"

// Configured as usual, outside of the macros changed below
#include <etl/error_handler.h>
#include <etl/platform.h>

#pragma push_macro("abs")
#undef abs

#if defined(ARDUINO_ARCH_ESP8266)
#pragma push_macro("NDEBUG")
#undef NDEBUG
#define NDEBUG
#include <assert.h> // assert(e) is ((void)0), here and in every <assert.h> included by etl/chrono.h
#endif

#include <etl/chrono.h>

#if defined(ARDUINO_ARCH_ESP8266)
#pragma pop_macro("NDEBUG")
#include <assert.h> // the assert of the application again
#endif

#pragma pop_macro("abs")

#endif // DUCTA_CORE_PRIVATE_ETL_CHRONO_HPP
