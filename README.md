# Ant Simulations (Pygame)

This folder contains multiple standalone Python simulations (e.g. `ant_sim.py`). Each simulation is a plain `*.py` file you can run directly.

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

## Run a simulation

The simulations are just Python files. To run `ant_sim.py`:

```powershell
cd "C:\Users\YOUR_NAME\Documents\python\ants_sim"
.\.venv\Scripts\python.exe .\ant_sim.py
```

To run a different simulation file in the same folder, replace the filename:

```powershell
.\.venv\Scripts\python.exe .\YOUR_SIMULATION.py
```

## Adding new simulations

- Add a new `*.py` file to this folder.
- If the new simulation needs extra packages, add them to `requirements.txt` (so everyone can install everything with one command).

