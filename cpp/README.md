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

## Add A Simulation

1. Add a new `.cpp` file under `src/simulations`.
2. Create a class that derives from `Simulation`.
3. Add a factory function for it.
4. Register it in `src/simulations/registry.cpp`.
5. Add its id to `SIM_IDS` in `Makefile` if you want a separate executable under `build/sims`.

The flight simulation is the current reference template. It mirrors a pygame-style interactive simulation layout with a starfield, craft, waypoint, autopilot toggle, and right-side diagnostics panel.

Example registry entry:

```cpp
{"my-sim", "My Simulation", "Category", "Short description.", &CreateMySimulation},
```

Example direct target:

```make
SIM_IDS := flight my-sim
```

## Advanced Windows Configuration

The project assumes the standard Raylib Windows bundle is installed at `C:\raylib`. You can set environment variables to override:

```powershell
$env:RAYLIB_INCLUDE = "D:\tools\raylib\include"
$env:RAYLIB_LIB = "D:\tools\raylib\lib\libraylib.a"
$env:RAYLIB_TOOLCHAIN_BIN = "D:\tools\w64devkit\bin"
.\build.ps1
```
