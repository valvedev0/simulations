# Raylib Scientific Simulations

A small C++ Raylib workspace for building scientific simulations and comparing them with implementations in other languages, such as Python.

## Layout

```text
include/
  simulation.hpp       Shared simulation interface
  simulation_app.hpp   Shared launcher/direct-run app shell
src/
  main.cpp             GUI launcher entry point
  standalone_main.cpp  Direct simulation executable entry point
  simulation_app.cpp   Window loop, menu, keyboard handling
  simulations/
    registry.cpp       Registers available simulations
    flight.cpp         Pygame-style 2D flight diagnostics experiment
    ants.cpp           Ant colony foraging diagnostics experiment
```

## Build (Windows)

Uses the MinGW toolchain that ships with the standard Raylib Windows bundle. From this folder:

```powershell
.\build.ps1
```

This builds the launcher and each direct simulation executable. If Raylib is not installed, use:

```powershell
.\install_deps.ps1
.\build.ps1
```

Run the GUI launcher:
```powershell
.\build\simulations.exe
```

## Build (Linux)

### 1. Install Dependencies

You will need `build-essential` and the Raylib development dependencies. On Ubuntu/Debian/Mint:

```bash
sudo apt update
sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

### 2. Install Raylib

If you haven't installed Raylib yet, follow the [Raylib Wiki](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) to build and install it from source, or use your package manager if a recent version is available.

### 3. Build

Simply run `make` in this folder:

```bash
make
```

Run the GUI launcher:
```bash
./build/simulations
```

Run a simulation directly:
```bash
./build/sims/flight
./build/sims/ants
```

## Controls

- `Up` / `Down`: choose in launcher
- `Enter`: start selected simulation
- `N` or `Right Arrow`: next simulation while running
- `P` or `Left Arrow`: previous simulation while running
- `R`: reset current simulation
- `Esc`: return to launcher

Flight controls:

- `W` / `S` or `Up` / `Down`: increase or decrease throttle
- `A` / `D` or `Left` / `Right`: turn
- `Left Mouse Button`: set waypoint
- `Right Mouse Button` or `P`: toggle autopilot
- `Space`: zero velocity
- `R`: reset
- `Esc`: quit direct executable, or return to launcher
## Build (WebAssembly)

This project supports compiling to WebAssembly (Wasm) using Emscripten, allowing simulations to run in a web browser.

### 1. Prerequisites

- **Emscripten (emsdk):** [Install and activate](https://emscripten.org/docs/getting_started/downloads.html) the Emscripten toolchain.
- **Local Web Server:** Needed to test the build (e.g., Python `http.server` or Node `npx serve`).

### 2. Build

Activate the Emscripten environment in your terminal and run the web build script:

```powershell
& "C:\path\to\emsdk\emsdk_env.ps1"
.\build_web.ps1
```

The script will:
1. Automatically compile a WebAssembly-compatible version of Raylib if not found.
2. Compile the simulations into `build/web/simulations.html`.

### 3. Run

Start a local server in the output directory:

```powershell
python -m http.server -d build/web
```
Navigate to `http://localhost:8000/simulations.html`.

## Controls
...
## Add A Simulation

1. Add a new `.cpp` file under `src/simulations`.
2. Create a class that derives from `Simulation`.
3. Add a factory function for it.
4. Register it in `src/simulations/registry.cpp`.
5. Add its id to `SIM_IDS` in `Makefile` if you want a separate executable under `build/sims`.

**Note for Web:** Ensure your simulation follows the `update(float deltaTime)` and `draw()` pattern. Do not introduce any blocking infinite loops inside these methods, as they will freeze the browser tab.

## Web Compatibility Guidelines

To ensure your simulations work perfectly on the web:

- **Main Loop:** Never create your own `while` loop for frames. Use the `update()` and `draw()` methods provided by the `Simulation` interface. The `SimulationApp` handles the platform-specific loop logic (Emscripten vs Desktop).
- **File I/O:** Browsers use a virtual file system. If your simulation needs to load assets (textures, data), they must be preloaded or embedded using Emscripten's `--preload-file` or `--embed-file` flags in `build_web.ps1`.
- **Performance:** While WebAssembly is fast, try to optimize complex physics or many-particle systems. Use `deltaTime` for all movement to ensure consistent behavior across different screen refresh rates.
- **Resolution:** The workspace is currently optimized for a `1220x640` canvas.

## Advanced Windows Configuration

The project assumes the standard Raylib Windows bundle is installed at `C:\raylib`. You can set environment variables to override:

```powershell
$env:RAYLIB_INCLUDE = "D:\tools\raylib\include"
$env:RAYLIB_LIB = "D:\tools\raylib\lib\libraylib.a"
$env:RAYLIB_TOOLCHAIN_BIN = "D:\tools\w64devkit\bin"
.\build.ps1
```
