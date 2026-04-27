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
    fluid.cpp          Interactive particle-based fluid simulation
    birds.cpp          Bird flocking with weather and predator logic
    rocket.cpp         Rocket landing game with PID-controlled descent
    galaxy.cpp         High-performance 50,000+ particle simulator using rlgl
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

### Rocket Landing
- `W`: Fire Main Engine (Manual)
- `A` / `D`: Rotation / RCS Thrusters
- `P`: Toggle Autopilot (PID Stabilization)
- `R`: Reset Mission

### Bird Flock & Weather
- `1` - `4`: Change Weather (Clear, Wind, Storm, Rain)
- `Space`: Spawn Predator at mouse position
- `Left Mouse Button`: Set target waypoint for flock
- `Right Mouse Button`: Set wind direction
- `+` / `-`: Change simulation speed

### Particle Fluid
- `Left Mouse Button`: Attract particles
- `Right Mouse Button`: Repel particles
- `R`: Reset fluid particles

### High-Performance Galaxy
- `Left Mouse Button (Hold)`: Drag the central black hole
- `R`: Regenerate the galaxy

## Build (WebAssembly)

This project supports compiling to WebAssembly (Wasm) using Emscripten, allowing simulations to run in a web browser.

### 1. Install Emscripten (emsdk)

If you don't have the Emscripten SDK installed:

1.  **Clone the repository:**
    ```powershell
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ```
2.  **Install and Activate:**
    ```powershell
    # Download and install the latest SDK tools.
    .\emsdk.ps1 install latest

    # Make the "latest" SDK "active" for the current user.
    .\emsdk.ps1 activate latest
    ```

### 2. Activate Environment

Every time you open a new terminal, you must activate the Emscripten environment variables:

```powershell
# Run this from your emsdk directory
.\emsdk_env.ps1
```

*Tip: You can add the emsdk directory to your PATH or run the activation script automatically in your profile to skip this step.*

### 3. Build

Once `emcc` is available in your path, run the web build script from the project root:

```powershell
.\build_web.ps1
```

If you are building for the first time or need to recompile the Raylib library itself (e.g., after an update), use the `-Force` flag:

```powershell
.\build_web.ps1 -Force
```

The script will:
1.  Automatically compile a WebAssembly-compatible version of Raylib (`libraylib_web.a`).
2.  Compile all simulations into `build/web/simulations.html`.

### 4. Run

Start a local server in the output directory (browsers cannot run Wasm files directly from `file://` URIs):

```powershell
# Using Python
python -m http.server -d build/web

# Or using Node.js
npx serve build/web
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
