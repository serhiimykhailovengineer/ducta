# ducta
Ducta — modular C++ data-flow framework for connecting and executing computational nodes.

## Building

Requirements: CMake ≥ 3.25, Conan 2, a C++17 compiler, and [just](https://github.com/casey/just) (optional).

The framework supports two backends. Each has a Release and a Debug CMake preset, built into `build/<preset>`:

| Preset      | Backend                        | Build type |
|-------------|--------------------------------|------------|
| `std`       | Standard (Boost, fmt)          | Release    |
| `etl`       | Embedded (ETL)                 | Release    |
| `etl-mcu`   | Embedded (ETL), MCU capacities | Release    |
| `std-debug` | Standard (Boost, fmt)          | Debug      |
| `etl-debug` | Embedded (ETL)                 | Debug      |

Conan dependencies are shared by all presets and installed into `build/conan`.

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
`include/ducta/Core/Capacity.hpp`). Every capacity is a CMake option; an empty option keeps the
value of the selected profile:

```bash
cmake --preset etl -DDUCTA_CAPACITY_PROFILE=mcu       # small capacities, no execution trace
cmake --preset etl -DDUCTA_MAX_NODES=32               # one capacity
cmake --preset etl -DDUCTA_ENABLE_TRACING=OFF         # no trace buffers and trace code
```

Without CMake, define `DUCTA_CAPACITY_PROFILE_MCU` (and any `DUCTA_*` capacity) for the compiler.
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
