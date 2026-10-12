# Ducta task runner — run `just` to list recipes.
#
# Every recipe takes an optional preset (default: std):
#   std       — standard backend (Boost + fmt), Release
#   etl       — embedded backend (ETL), Release
#   std-debug — standard backend, Debug
#   etl-debug — embedded backend, Debug
#   all       — all of the above, one after another

# List available recipes
default:
    @just --list

# Install Conan dependencies and configure CMake
configure preset="std":
    ./project/configure.sh {{preset}}

# Build all targets
build preset="std":
    #!/usr/bin/env bash
    set -e
    for b in {{ if preset == "all" { "std etl std-debug etl-debug" } else { preset } }}; do
        cmake --build --preset "$b"
    done

# Run tests, optionally filtered by regex (e.g. `just test etl Pipeline`)
test preset="std" filter="":
    #!/usr/bin/env bash
    set -e
    for b in {{ if preset == "all" { "std etl std-debug etl-debug" } else { preset } }}; do
        ctest --preset "$b" {{ if filter != "" { "-R " + quote(filter) } else { "" } }}
    done

# Build the tests, then run them
check preset="std":
    #!/usr/bin/env bash
    set -e
    for b in {{ if preset == "all" { "std etl std-debug etl-debug" } else { preset } }}; do
        cmake --build --preset "$b" --target check
    done

# Configure, build and test from scratch
all preset="all": (configure preset) (check preset)

# Remove build directories (also removes Conan dependencies when preset is `all`)
clean preset="all":
    rm -rf {{ if preset == "all" { "build" } else { "build/" + preset } }}

# Start the Linux development container
container:
    ./project/run_container.sh
