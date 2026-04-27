# Python Simulations

This repository contains a small collection of interactive Python simulations built with `pygame`, along with a launcher and a lightweight static web-export workflow for selected sims.

## Simulations

### Ant Simulation

The ant simulation visualizes groups of ants routing toward separate bases while steering around obstacles. It is useful as a compact movement-behavior demo because it exposes pathing, obstacle avoidance, and arrival analytics in the same scene.

### Flight Simulation

The flight simulation is a simple 2D aircraft sandbox with throttle, turning, waypoint targeting, and optional autopilot. It focuses on continuous motion, heading control, and diagnostic telemetry rather than arcade combat or terrain.

### Laser Puzzle

The laser simulation is a simpler mirror-reflection puzzle. The goal is to aim a visible laser pulse, bounce it off mirrors, remove green targets, and avoid moving red drones. Hitting a drone ends the round, later levels grow the target safety circles, the beam pulse travels more slowly, and each round is governed by a countdown timer.

## Project Structure

```text
python/
  build_web.bat
  sim_launcher.py
  Makefile
  requirements.txt
  build/
  tools/
    build_web_sim.py
  web_templates/
    laser/
      app.js
      styles.css
  simulations/
    ant/
      ant_sim.py
      run.py
    flight/
      flight_sim.py
      run.py
    laser/
      laser_sim.py
      run.py
```

## Requirements

- Python 3.12+
- `pygame`
- `uv` is recommended, but optional

On Linux, you may also need the system package for `tkinter` if you want to run the desktop launcher.

## Setup

### Option 1: `uv` (recommended)

**Windows**

```powershell
uv venv --clear --python 3.12 .venv
uv pip install -r requirements.txt --only-binary :all:
```

**Linux/macOS**

```bash
uv venv --clear --python 3.12 .venv
uv pip install -r requirements.txt
```

### Option 2: standard Python

**Windows**

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

**Linux/macOS**

```bash
python3 -m venv .venv
./.venv/bin/python -m pip install --upgrade pip
./.venv/bin/python -m pip install -r requirements.txt
```

## Running the desktop sims

### Launcher

Run the launcher from the project root:

**Windows**

```powershell
.\.venv\Scripts\python.exe sim_launcher.py
```

**Linux/macOS**

```bash
./.venv/bin/python sim_launcher.py
```

### Run a specific sim directly

**Ant**

```powershell
.\.venv\Scripts\python.exe simulations\ant\run.py
```

```bash
./.venv/bin/python simulations/ant/run.py
```

**Flight**

```powershell
.\.venv\Scripts\python.exe simulations\flight\run.py
```

```bash
./.venv/bin/python simulations/flight/run.py
```

**Laser**

```powershell
.\.venv\Scripts\python.exe simulations\laser\run.py
```

```bash
./.venv/bin/python simulations/laser/run.py
```

## Static Web Builds

Selected simulations can be exported as self-contained static web folders. These exports are designed for copy-and-paste deployment to GitHub Pages or any other static host.

The maintained web-export target in this repo today is `laser`.

### List supported web builds

```powershell
.\.venv\Scripts\python.exe tools\build_web_sim.py --list
```

```bash
./.venv/bin/python tools/build_web_sim.py --list
```

### Build a specific sim

```powershell
.\.venv\Scripts\python.exe tools\build_web_sim.py laser
```

```bash
./.venv/bin/python tools/build_web_sim.py laser
```

### Windows shortcut

```powershell
.\build_web.bat laser
```

This generates the laser export under `build/laser_Sim webbuild/`.

The current laser web build follows the same simplified rules as the desktop sim: clear green targets, avoid red drones, and use the sidebar to track targets left, hazard count, safety-circle size, beam speed, and time remaining.

### Makefile shortcut

```bash
make web-build SIM=laser
```

### Publish workflow

1. Generate the export locally.
2. Open the output folder under `build/` and verify the files you want to publish.
3. Copy the generated contents into the root or publish directory of your GitHub Pages repo.
4. Commit and push those files.

The generated web package uses only relative paths, so it does not need Python, `pygame`, or a frontend build tool after export.

### Output layout

For the current laser export, the generated folder is:

```text
build/
  laser_Sim webbuild/
    index.html
    styles.css
    app.js
    README.md
```

### Source files for web builds

The `build/` folder is generated output. The source files you edit live elsewhere in the repo:

- Browser runtime logic lives under `web_templates/<sim>/`
- Build metadata and output generation live in `tools/build_web_sim.py`
- The original desktop sim stays under `simulations/<sim>/`

### Why `build/` is ignored in Git

The generated web files under `build/` are intentionally ignored in `.gitignore`.

They are excluded because:

- they are generated artifacts, not the main source of truth
- the same files can be rebuilt locally from `tools/build_web_sim.py`
- developers may generate them on different PCs, operating systems, or shells
- line endings, timestamps, local verification steps, or rebuild timing can differ across environments
- keeping generated output out of Git helps avoid noisy diffs and accidental commits

In this workflow, the editable source lives in:

- `simulations/<sim>/` for the desktop Python version
- `web_templates/<sim>/` for the browser implementation
- `tools/build_web_sim.py` for export metadata and generation logic

The `build/` folder should be treated as temporary publishable output that you regenerate when needed.

## Web build source templates

The generated files in `build/` are output artifacts. The maintained source files for browser exports live under `web_templates/`.

For the current laser export:

- Browser runtime source: `web_templates/laser/app.js`
- Browser styling source: `web_templates/laser/styles.css`
- Builder script: `tools/build_web_sim.py`

If you want to add web support for another sim later, add a template folder under `web_templates/<sim>/` and register it in `tools/build_web_sim.py`.

## Adding new simulations

To add a new desktop simulation:

1. Create a new folder under `simulations/`.
2. Add the simulation file and a `run.py` entry point.
3. Install any new Python dependencies in `requirements.txt`.

To add a new web-export target:

1. Choose a short slug for the sim, such as `swarm`, `orbit`, or `maze`.
2. Create a browser source folder under `web_templates/<slug>/`.
3. Add the browser files needed by the exporter.
   At minimum, this project currently expects:
   - `web_templates/<slug>/app.js`
   - `web_templates/<slug>/styles.css`
4. Implement the browser version in plain web technologies.
   The current workflow is designed for static hosting, so prefer:
   - HTML generated by `tools/build_web_sim.py`
   - plain JavaScript in `app.js`
   - plain CSS in `styles.css`
   Avoid build-tool-only assumptions unless you also extend the exporter.
5. Open `tools/build_web_sim.py` and add a new `SimBuild(...)` entry to the `SIM_BUILDS` registry.
   That entry should define:
   - `slug`: command name used by the builder
   - `display_name`: human-readable title
   - `output_dir_name`: folder name to create under `build/`
   - `source_python_path`: original desktop sim path
   - `template_dir_name`: matching folder under `web_templates/`
   - `tagline`, `objective`, `play_notes`, and `tech_notes`: text shown in generated docs/page content
6. Generate the web build locally from the project root.

**Windows**

```powershell
.\build_web.bat <slug>
```

or

```powershell
.\.venv\Scripts\python.exe tools\build_web_sim.py <slug>
```

**Linux/macOS**

```bash
./.venv/bin/python tools/build_web_sim.py <slug>
```

7. Check the generated output under `build/`.
   Confirm that:
   - the expected folder was created
   - `index.html`, `styles.css`, `app.js`, and `README.md` are present
   - relative paths work correctly
   - the sim runs in a browser without needing Python or extra tooling
8. If you are publishing to GitHub Pages or another static host, copy the contents of the generated folder into the site repo or publish directory.

### Suggested workflow for a new web target

1. Start from an existing desktop sim under `simulations/<slug>/`.
2. Port the core gameplay into `web_templates/<slug>/app.js`.
3. Add styling in `web_templates/<slug>/styles.css`.
4. Register the target in `tools/build_web_sim.py`.
5. Generate the output under `build/`.
6. Test locally in a browser.
7. Publish the generated files.

### Notes for cross-platform use

- Keep paths in docs and code relative to the repo root.
- Prefer forward-slash paths in documentation examples when possible, except for Windows-specific command examples.
- Use the venv Python executable shown for each platform instead of assuming a global `python` command is configured the same way everywhere.
- Treat `build/` as generated output and `web_templates/` as the editable source for browser exports.
