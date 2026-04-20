# Python Simulations (Pygame)

This project contains multiple standalone Python simulations and a root launcher GUI.

## Project structure

```text
python/
  sim_launcher.py               # Root launcher GUI
  configs/
    launcher_config.json        # Launcher UI config
  simulations/
    ant/
      ant_sim.py                # Ant simulation
      run.py                    # Entry point used by launcher
    flight/
      flight_sim.py             # Flight simulation
      run.py                    # Entry point used by launcher
    laser/
      laser_sim.py              # Laser simulation
      run.py                    # Entry point used by launcher
```

## Prerequisites

- Python 3.12+
- `uv` (recommended for creating the environment and installing dependencies)
- On Linux, you may need to install `tkinter` for the GUI launcher (e.g., `sudo apt install python3-tk`).

### Install `uv`

**Windows (PowerShell):**
```powershell
winget install --id=astral-sh.uv -e
```

**Linux/macOS:**
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Verify installation:
```bash
uv --version
```

## Setup (Environment & Dependencies)

### 1. Using `uv` (Recommended)

**Windows:**
```powershell
uv venv --clear --python 3.12 .venv
uv pip install -r requirements.txt --only-binary :all:
```

**Linux/macOS:**
```bash
uv venv --clear --python 3.12 .venv
uv pip install -r requirements.txt
```

### 2. Using standard Python (No `uv`)

If you don't have `uv` installed, you can use the built-in `venv` module.

**Windows:**
```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

**Linux/macOS:**
```bash
python3 -m venv .venv
./.venv/bin/python -m pip install --upgrade pip
./.venv/bin/python -m pip install -r requirements.txt
```

*Notes:*
- `.venv` is a project-local virtual environment that keeps dependencies isolated.
- On Linux, ensure `python3-venv` is installed (`sudo apt install python3-venv`).
- If you use the standard `python` method on Windows and encounter build errors, you may need the "Build Tools for Visual Studio". `uv` avoids this by using pre-built binaries.

## Run from launcher

Start the GUI launcher from the project root:

**Windows:**
```powershell
.\.venv\Scripts\python.exe sim_launcher.py
```

**Linux/macOS:**
```bash
./.venv/bin/python sim_launcher.py
```

## Run a simulation directly

**Windows:**
```powershell
.\.venv\Scripts\python.exe simulations\ant\run.py
.\.venv\Scripts\python.exe simulations\flight\run.py
.\.venv\Scripts\python.exe simulations\laser\run.py
```

**Linux/macOS:**
```bash
./.venv/bin/python simulations/ant/run.py
./.venv/bin/python simulations/flight/run.py
./.venv/bin/python simulations/laser/run.py
```

## Adding new simulations (clean pattern)

- Create a new folder under `simulations/`, for example `simulations/swarm/`.
- Add a `run.py` in that folder as the simulation entry point.
- The launcher auto-discovers simulations by scanning `simulations/*/run.py`.
- If a new simulation needs extra packages, add them to `requirements.txt`.

