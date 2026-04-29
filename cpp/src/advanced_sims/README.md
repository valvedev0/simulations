# Advanced GPU Simulations

This directory contains standalone, highly advanced simulations that bypass the standard Raylib framework and WebAssembly restrictions to leverage raw GPU compute power (OpenGL 4.3+ Compute Shaders).

These simulations are meant for desktop only (Windows/Linux) and are designed to push dedicated GPUs (like RTX 4060, GTX 1060) to their limits.

## 1. 2-Million Particle GPU Physics (`gpu_particles.cpp`)

This simulation offloads 100% of the physics math and rendering logic to the GPU. The CPU does not track the particles at all after initial generation. 

### How it works:
- **Compute Shader:** Calculates gravity, tangential orbiting, velocity, wrapping, and dynamic color changes for all 2,000,000 particles simultaneously across thousands of GPU cores.
- **SSBO (Shader Storage Buffer Object):** A massive block of GPU memory that holds the particles.
- **Vertex Shader Rendering:** The rendering shader reads directly from the SSBO, drawing 2 million pixels instantly without passing data back and forth to the CPU.

### Controls & Interactivity
- **Left Mouse Button (Hold):** Creates a gravity well (attractor) that pulls all particles toward the mouse, creating swirling accretion disks.
- **Right Mouse Button (Hold):** Triggers a "Supernova" repel force, blasting all particles away.
- **Scroll Wheel:** Increases or decreases the overall gravity force multiplier.
- **Spacebar:** Toggles between 3 different Shader Color Modes:
  1. `Plasma Fire` (Red/Orange based on speed)
  2. `Cyberpunk Neon` (Pink/Blue/Purple)
  3. `Cosmic Aurora` (Dynamic, time-and-position-based glowing patterns)

---

## Build Instructions

Because this is an advanced, standalone simulation, it uses its own build scripts to keep the main web-compatible workspace clean.

### Windows (Requires `w64devkit`)
Ensure your `w64devkit` is installed at `C:\raylib\w64devkit` (or edit the script paths).
```powershell
# From the project root:
cd src/advanced_sims
.\build_windows.ps1

# Run it:
..\..\build\advanced\gpu_particles.exe
```
*Note: The C++ file contains explicit DLL export hints to force Windows to use the dedicated NVIDIA/AMD GPU instead of the integrated Intel graphics.*

### Linux (Requires standard build tools and Raylib)
```bash
# From the project root:
cd src/advanced_sims
make

# Run it:
../../build/advanced/gpu_particles
```
