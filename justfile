set shell := ["bash", "-uc"]

# Show available commands
default:
    @just --list

# Format all code using treefmt
fmt:
    treefmt --allow-missing-formatter

# Check formatting without modifying files
check-formatted:
    treefmt --allow-missing-formatter --fail-on-change

# Configure and build (release)
build:
    cmake --preset release
    cmake --build --preset release

# Configure and build (debug)
build-debug:
    cmake --preset debug
    cmake --build --preset debug

# Build and run all Qt tests headlessly
test:
    cmake --preset test
    cmake --build --preset test
    ctest --preset test

# Run all CI checks (formatting + tests)
ci: check-formatted test

# Remove all build and release artifacts
clean:
    rm -rf build dist
