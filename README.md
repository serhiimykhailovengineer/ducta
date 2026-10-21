# ducta
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
Every source file that includes Ducta checks this before `main()` and calls `ducta::panic` if
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

The dependencies are found with `find_package` (`etl`, or `Boost` and `fmt`), unless the project
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
profile's default). With CMakeDeps: `find_package(ducta)` and `ducta::ducta`. The package is
checked by `test_package/`.

### PlatformIO

```ini
; platformio.ini
lib_deps = https://github.com/serhiimykhailovengineer/ducta.git#v0.1.0
build_unflags = -std=gnu++11 -std=gnu++14
build_flags = -std=gnu++17 -I include    ; include/ducta_config.hpp, for the libraries too
```

PlatformIO selects the embedded backend and installs ETL. A complete program (a board and the
computer as `native`) is in `examples/platformio/`.

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

### VS Code

Requires the C/C++ extension (`ms-vscode.cpptools`).

- **Tasks** (`Terminal → Run Task`): `Configure`, `Build` (default, `Cmd/Ctrl+Shift+B`), `Test` (default test task, takes a regex filter), `Build and Test`, `Clean`. Each asks for a preset.
- **Debugging** (`Run and Debug`), using lldb on macOS and gdb on Linux:
  - `Launch test (std-debug)` / `Launch test (etl-debug)` builds the Debug preset, then runs the test executable you pick with an optional `--gtest_filter`.
  - `Attach to process` attaches to a running test process; choose the preset and executable it was built from.
