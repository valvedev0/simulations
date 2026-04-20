#!/bin/bash
set -e

# Ensure emcc is available
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Please install and activate Emscripten (emsdk)."
    exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RAYLIB_INCLUDE="${RAYLIB_INCLUDE:-/usr/local/include}"
RAYLIB_WEB_LIB="${RAYLIB_WEB_LIB:-$PROJECT_ROOT/build/web/libraylib.a}"

# Note: Building Raylib for Web from source on Linux requires downloading the Raylib source
# and running `make PLATFORM=PLATFORM_WEB` inside its `src` directory.
# Assuming the user has a pre-built libraylib.a for Web or we use the system one if it's wasm.
# For a full robust script, one would clone Raylib. Here we assume RAYLIB_WEB_LIB exists
# or the user has compiled raylib for web.

if [ ! -f "$RAYLIB_WEB_LIB" ]; then
    echo "Warning: libraylib.a for WebAssembly not found at $RAYLIB_WEB_LIB."
    echo "Please compile Raylib for Web using emscripten and set RAYLIB_WEB_LIB to its path."
    echo "Example: git clone https://github.com/raysan5/raylib.git"
    echo "         cd raylib/src && make PLATFORM=PLATFORM_WEB"
    echo "         cp libraylib.a $PROJECT_ROOT/build/web/"
    exit 1
fi

mkdir -p "$PROJECT_ROOT/build/web"

# Find all simulation sources
SIM_SOURCES=$(find "$PROJECT_ROOT/src/simulations" -name "*.cpp")

echo "Building Raylib workspace for WebAssembly..."

emcc -std=c++17 -Wall \
    -Iinclude -I"$RAYLIB_INCLUDE" \
    -s USE_GLFW=3 -s ASYNCIFY \
    --shell-file "$PROJECT_ROOT/minshell.html" \
    src/main.cpp src/simulation_app.cpp $SIM_SOURCES \
    "$RAYLIB_WEB_LIB" \
    -o "$PROJECT_ROOT/build/web/simulations.html"

echo ""
echo "Build successful!"
echo "Web build created at: $PROJECT_ROOT/build/web/simulations.html"
echo "To test, run a local web server:"
echo "  python3 -m http.server -d build/web"
