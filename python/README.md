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

## Prerequisites (Windows)

- Python 3.12+
- `uv` (recommended for creating the environment and installing dependencies)

### Install `uv`

Run in PowerShell:

```powershell
winget install --id=astral-sh.uv -e
```

Verify:

```powershell
uv --version
```

## Setup (install dependencies once)

From this folder:

```powershell
cd "C:\Users\YOUR_NAME\Documents\python\ants_sim"
uv venv --clear --python 3.12 .venv
uv pip install -r requirements.txt --only-binary :all:
```

Notes:
- `venv`/`.venv` is a project-local virtual environment (keeps installs from breaking your global Python).
- The `--only-binary :all:` flag avoids building packages from source on Windows.

## Run from launcher

Start the GUI launcher from project root:

```powershell
cd "C:\Users\YOUR_NAME\Documents\python\ants_sim"
.\.venv\Scripts\python.exe .\sim_launcher.py
```

## Run a simulation directly

```powershell
.\.venv\Scripts\python.exe .\simulations\ant\run.py
.\.venv\Scripts\python.exe .\simulations\flight\run.py
.\.venv\Scripts\python.exe .\simulations\laser\run.py
```

## Adding new simulations (clean pattern)

- Create a new folder under `simulations/`, for example `simulations/swarm/`.
- Add a `run.py` in that folder as the simulation entry point.
- The launcher auto-discovers simulations by scanning `simulations/*/run.py`.
- If a new simulation needs extra packages, add them to `requirements.txt`.

