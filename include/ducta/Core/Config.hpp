/**
 * @file Config.hpp
 * @brief Build configuration of Ducta; every Ducta header includes it first
 *
 * The application configures Ducta in one header, ducta_config.hpp, found in its include path
 * (like FreeRTOSConfig.h or lwipopts.h). Every source file that includes a Ducta header then
 * sees the same configuration:
 *
 *     // ducta_config.hpp
 *     #pragma once
 *     #define DUCTA_USE_EMBEDDED_BACKEND        // or DUCTA_USE_STD_BACKEND (the default)
 *     #define DUCTA_CAPACITY_PROFILE_MCU        // small capacities, no execution trace
 *     #define DUCTA_MAX_NODES 24                // any capacity of Capacity.hpp
 *
 * DUCTA_CONFIG_FILE names another header instead (e.g. -DDUCTA_CONFIG_FILE="\"my_ducta.hpp\""),
 * DUCTA_NO_CONFIG_FILE skips it. Compile definitions work as well (CMake options, Conan options).
 */

#ifndef DUCTA_CORE_CONFIG_HPP
#define DUCTA_CORE_CONFIG_HPP

// ========== application configuration ==========
#if defined(DUCTA_CONFIG_FILE)
#include DUCTA_CONFIG_FILE
#elif !defined(DUCTA_NO_CONFIG_FILE) && defined(__has_include)
#if __has_include(<ducta_config.hpp>)
#include <ducta_config.hpp>
#endif
#endif

// ========== backend ==========
// DUCTA_USE_EMBEDDED_BACKEND: ETL (fixed capacity containers, no heap)
// DUCTA_USE_STD_BACKEND: standard library, Boost and fmt
// Without a choice: the embedded backend with PlatformIO, the standard one otherwise.
#if defined(DUCTA_USE_EMBEDDED_BACKEND) && defined(DUCTA_USE_STD_BACKEND)
#error "Ducta: define only one of DUCTA_USE_EMBEDDED_BACKEND and DUCTA_USE_STD_BACKEND"
#endif
#if !defined(DUCTA_USE_EMBEDDED_BACKEND) && !defined(DUCTA_USE_STD_BACKEND)
#if defined(PLATFORMIO)
#define DUCTA_USE_EMBEDDED_BACKEND
#else
#define DUCTA_USE_STD_BACKEND
#endif
#endif

// ========== declaration specifier for non-template definitions in .inl files ==========
// Header-only (default): definitions are inline so every TU may include them.
// DUCTA_SEPARATE_COMPILATION: definitions are compiled once in a single source file.
#if defined(DUCTA_SEPARATE_COMPILATION)
#define DUCTA_DECL
#else
#define DUCTA_DECL inline
#endif

// Capacities, and the check that the whole program uses the same ones
#include "ducta/Core/Capacity.hpp"

#endif // DUCTA_CORE_CONFIG_HPP
