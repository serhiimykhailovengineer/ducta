# ducta

[![CI](https://github.com/serhiimykhailovengineer/ducta/actions/workflows/ci.yml/badge.svg)](https://github.com/serhiimykhailovengineer/ducta/actions/workflows/ci.yml)

Ducta — modular C++ data-flow framework for connecting and executing computational nodes.

Header-only, C++17. Two backends: `std` (standard library, Boost, fmt) and `etl` (Embedded
Template Library: fixed capacities, no heap, works with `-fno-exceptions`).

## Using Ducta in a project

### Configuration: `ducta_config.hpp`

The application configures Ducta in one header named `ducta_config.hpp`, placed in an include
directory seen by **every** source file that includes Ducta (see `include/ducta/Core/Config.hpp`
and `include/ducta/Core/Capacity.hpp`):

```cpp
// ducta_config.hpp
#pragma once
#define DUCTA_USE_EMBEDDED_BACKEND   // or DUCTA_USE_STD_BACKEND (default; PlatformIO: embedded)
#define DUCTA_CAPACITY_PROFILE_MCU   // small capacities, no execution trace
#define DUCTA_MAX_NODES 24           // any capacity
```

The capacities change the layout of Ducta types, so the whole program must use the same ones.
Every source file that includes Ducta checks this before `main()` and calls `ducta::fatal_error` if
another file was compiled with other capacities or another backend (`DUCTA_NO_CAPACITY_CHECK`
removes the check). Set each value in one place: `ducta_config.hpp`, or the compile definitions
of the package (CMake / Conan options below).

The embedded backend needs two platform functions from the application, unless
`DUCTA_USE_HOST_PLATFORM` provides them on a computer: `etl_get_steady_clock()` and
`ducta_sleep_for_us()` (see `examples/platformio/src/platform.cpp`).

### CMake: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(ducta
    GIT_REPOSITORY https://github.com/serhiimykhailovengineer/ducta.git
    GIT_TAG v0.1.0)
set(DUCTA_BACKEND etl)              # std (default) or etl
FetchContent_MakeAvailable(ducta)

target_link_libraries(app PRIVATE ducta::ducta)
```

The dependencies are found with `find_package` (`etl` 20.46 or newer, or `Boost` and `fmt`), unless the project
already defines their targets (`etl::etl`, `Boost::headers`, `fmt::fmt`), e.g. with FetchContent
as well. Tests are built only when Ducta is the top-level project (`DUCTA_BUILD_TESTS`).

### CMake: installed package

```bash
cmake -S ducta -B build -DDUCTA_BACKEND=etl -DDUCTA_BUILD_TESTS=OFF
cmake --install build --prefix /opt/ducta
```

```cmake
find_package(ducta 0.1 CONFIG REQUIRED)
target_link_libraries(app PRIVATE ducta::ducta)
```

### Conan

```bash
conan create . -o "ducta/*:backend=etl"   # into the local cache (or upload to your remote)
```

```python
# conanfile.py of the application
requires = "ducta/0.1.0"
default_options = {"ducta/*:backend": "etl", "ducta/*:capacity_profile": "mcu"}
```

Options: `backend` (`std`, `etl`), `capacity_profile` (`default`, `mcu`), `tracing` (unset: the
profile's default), `logging` (`True`, `False`). With CMakeDeps: `find_package(ducta)` and `ducta::ducta`. The package is
checked by `test_package/`.

### PlatformIO

```ini
; platformio.ini
lib_deps = https://github.com/serhiimykhailovengineer/ducta.git#v0.1.0
build_unflags = -std=gnu++11 -std=gnu++14
build_flags = -std=gnu++17 -I include    ; include/ducta_config.hpp, for the libraries too
```

PlatformIO selects the embedded backend and installs ETL. A complete program (STM32, ESP32,
ESP8266, Arduino UNO R4 and the computer as `native`) is in `examples/platformio/`.

### Logging

Ducta and the nodes log through `ducta::Log::logger()` (`ducta/Utils/Log.hpp`). Records go to
the attached sinks; without sinks nothing is written. Each sink has its own header in
`ducta/Utils/Log/`, so only the included ones are compiled in:

| Header | Writes to |
|---|---|
| `ConsoleSink.hpp` | stdout, warnings and errors to stderr |
| `FileSink.hpp` | a file it opens, or any `std::FILE*` |
| `CallbackSink.hpp` | a function of the application (a UART driver, a queue, UDP, ...) |
| `PrintSink.hpp` | an Arduino `Print` (`Serial`, a TCP client, ...) |
| `TextSink.hpp` | base (CRTP) of the above: one text line per record, to write anywhere else |

```cpp
#include "ducta/Utils/Log/ConsoleSink.hpp"
#include "ducta/Utils/Log/FileSink.hpp"

ducta::Log::ConsoleSink console;
ducta::Log::FileSink file{"pipeline.log"};
ducta::Log::logger().add_sink(console);                          // info and above (the default level)
ducta::Log::logger().add_sink(file, ducta::Log::Level::Warning); // warnings and errors only
ducta::Log::logger().set_time_source(+[] { return ducta::Chrono::SteadyClock{}.now(); });

DUCTA_LOG_INFO("node '{}' created", name);   // [12.345678] [info] create:42: node 'gain' created
```

A sink needs no base class: any type with
`void write(Log::Level, StringView message, char const* function, uint32_t line, Chrono::TimestampUS timestamp)`
(and, optionally, `void flush()`) can be attached. The logger keeps its address and one function
per sink type; it does not own it.

A message is formatted only when a sink takes its level, into `DUCTA_LOG_MESSAGE_LENGTH`
characters on the stack. `DUCTA_LOG_LEVEL` (0 debug ... 4 off; CMake `-DDUCTA_LOG_LEVEL=warning`)
removes the less important calls from the program. `DUCTA_ENABLE_LOGGING 0` (CMake
`-DDUCTA_ENABLE_LOGGING=OFF`, Conan `logging=False`) builds without logging: the calls compile
to nothing, their arguments are not evaluated, and the logger does nothing; create your sinks
under `#if DUCTA_ENABLE_LOGGING` so that they are not compiled in either. The logger is not
thread-safe yet.

## Developing Ducta

Requirements: CMake ≥ 3.25, Conan 2, a C++17 compiler, and [just](https://github.com/casey/just) (optional).

The framework supports two backends. Each has a Release and a Debug CMake preset, built into `build/<preset>`:

| Preset      | Backend                        | Build type |
|-------------|--------------------------------|------------|
| `std`       | Standard (Boost, fmt)          | Release    |
| `etl`       | Embedded (ETL)                 | Release    |
| `etl-mcu`   | Embedded (ETL), MCU capacities | Release    |
| `std-debug` | Standard (Boost, fmt)          | Debug      |
| `etl-debug` | Embedded (ETL)                 | Debug      |

Conan dependencies (of every backend, and the tests: `conanfile.py` with `develop=True`) are
shared by all presets and installed into `build/conan`.

```bash
just configure [preset|all]    # install Conan deps + configure CMake (default: std)
just build     [preset|all]    # build
just test      [preset|all]    # run tests
just test etl Pipeline         # run tests matching a regex
just check     [preset|all]    # build tests, then run them
just all                       # configure + build + test every preset
just clean     [preset|all]    # remove build dirs (default: all)
```

Without `just`, the equivalent commands are:

```bash
./project/configure.sh std                  # or: etl, etl-mcu, std-debug, etl-debug, all
cmake --build --preset std
ctest --preset std
cmake --workflow --preset std               # configure + build + test (after conan install)
```

### Capacities and microcontrollers

The embedded backend sizes its fixed containers with compile-time capacities (see
`include/ducta/Core/Capacity.hpp`). Every capacity is a CMake option (a compile definition of
`ducta::ducta`); an empty option keeps the value of the selected profile:

```bash
cmake --preset etl -DDUCTA_CAPACITY_PROFILE=mcu       # small capacities, no execution trace
cmake --preset etl -DDUCTA_MAX_NODES=32               # one capacity
cmake --preset etl -DDUCTA_ENABLE_TRACING=OFF         # no trace buffers and trace code
```

Without CMake, define them in `ducta_config.hpp` (see above).
The library compiles with `-fno-exceptions`: misuse such as reading an input that is not ready
then calls `DUCTA_PANIC_HANDLER(message)` if the application defines it, and `std::abort()`. With
the standard backend and `-fno-exceptions` the application defines `boost::throw_exception`
(see `tests/no_exceptions/NoExceptionsApp.cpp`).

### Continuous integration

`.github/workflows/ci.yml` builds and tests the Release presets `std`, `etl` and `etl-mcu` on
Linux and macOS for every pull request and every push to `main`, with the same commands as above. It also
builds the PlatformIO example for every environment of `examples/platformio/platformio.ini`
(the computer, STM32 Nucleo-F446RE, ESP32, ESP8266, Arduino UNO R4 Minima) and runs the native one.

### VS Code

Requires the C/C++ extension (`ms-vscode.cpptools`).

- **Tasks** (`Terminal → Run Task`): `Configure`, `Build` (default, `Cmd/Ctrl+Shift+B`), `Test` (default test task, takes a regex filter), `Build and Test`, `Clean`. Each asks for a preset.
- **Debugging** (`Run and Debug`), using lldb on macOS and gdb on Linux:
  - `Launch test (std-debug)` / `Launch test (etl-debug)` builds the Debug preset, then runs the test executable you pick with an optional `--gtest_filter`.
  - `Attach to process` attaches to a running test process; choose the preset and executable it was built from.
