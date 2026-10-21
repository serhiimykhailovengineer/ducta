#!/bin/bash
# Installs Conan dependencies and configures CMake for the given backend(s).
#
# Usage: ./project/configure.sh [std|etl|etl-mcu|std-debug|etl-debug|all]   (default: std)
#
# Afterwards everything is driven by CMake presets
# (<preset> = std | etl | etl-mcu | std-debug | etl-debug):
#   cmake --build --preset <preset>                  # build
#   ctest --preset <preset>                          # run tests
#   cmake --build --preset <preset> --target check   # build + run tests
#   cmake --workflow --preset <preset>               # configure + build + test

set -e

cd "$(dirname "$0")/.."

PRESET=${1:-std}
case "$PRESET" in
    std|etl|etl-mcu|std-debug|etl-debug) PRESETS=("$PRESET") ;;
    all) PRESETS=(std etl etl-mcu std-debug etl-debug) ;;
    *) echo "Unknown preset '$PRESET' (expected: std, etl, etl-mcu, std-debug, etl-debug, all)" >&2; exit 1 ;;
esac

# Dependencies are shared by all presets and installed once into build/conan.
# Debug presets link against the Release dependencies.
# develop=True: the dependencies of every backend and the tests (see conanfile.py).
conan install . --output-folder=build/conan --build=missing -s build_type=Release \
    -o "&:develop=True" -c tools.cmake.cmaketoolchain:user_presets=

for p in "${PRESETS[@]}"; do
    cmake --preset "$p" --fresh
done
