VENV_PY := .venv\Scripts\python.exe
ifeq ($(wildcard $(VENV_PY)),)
PY ?= python
else
PY ?= $(VENV_PY)
endif
SCRIPT ?= ant_sim.py

.PHONY: install install-all run

install:
	$(PY) -m pip install --upgrade pip
	$(PY) -m pip install -r requirements.txt

install-all:
	$(PY) -m pip install --upgrade pip
	$(PY) -m pip install -r requirements.txt

run:
	$(PY) $(SCRIPT)
