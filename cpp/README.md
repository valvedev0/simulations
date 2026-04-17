# Raylib Scientific Simulations

A small C++ Raylib workspace for building scientific simulations and comparing them with implementations in other languages, such as Python. It uses the MinGW toolchain that ships with the standard Raylib Windows bundle.

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

## Build

From this folder:

```powershell
.\build.ps1
```

This builds the launcher and each direct simulation executable.

If Raylib is not installed on the machine, install portable local dependencies first:

```powershell
.\install_deps.ps1
.\build.ps1
```

This downloads Raylib and W64Devkit into `.deps/`. No admin install is required.

Run the GUI launcher:

```powershell
.\build\simulations.exe
```

Run a simulation directly:

```powershell
.\build\sims\flight.exe
.\build\sims\ants.exe
```

You can also build or run make targets through the script:

```powershell
.\build.ps1 list
.\build.ps1 run-flight
.\build.ps1 run-ants
```

Controls:

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

Flight diagnostics shown in the window:

- elapsed time, frame count, FPS
- average frame, update, and render milliseconds
- position, speed, heading, throttle, autopilot state
- total distance, waypoint distance, max speed
- input/event counters for keyboard and mouse
- in-window controls panel
- after target reached, click `Continue` to resume from the paused state

The project assumes the standard Raylib Windows bundle is installed at:

```text
C:\raylib
```

The build also supports the repo-local `.deps/` installed by `install_deps.ps1`. If your Raylib folder moves, set explicit paths before building:

```powershell
$env:RAYLIB_INCLUDE = "D:\tools\raylib\include"
$env:RAYLIB_LIB = "D:\tools\raylib\lib\libraylib.a"
$env:RAYLIB_TOOLCHAIN_BIN = "D:\tools\w64devkit\bin"
.\build.ps1
```

You can also call the bundled make directly:

```powershell
C:\raylib\w64devkit\bin\mingw32-make.exe
```

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
