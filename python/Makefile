VENV_PY := .venv\Scripts\python.exe
ifeq ($(wildcard $(VENV_PY)),)
PY ?= python
else
PY ?= $(VENV_PY)
endif
SCRIPT ?= ant_sim.py
SIM ?= laser

.PHONY: install install-all run web-build

install:
	$(PY) -m pip install --upgrade pip
	$(PY) -m pip install -r requirements.txt

install-all:
	$(PY) -m pip install --upgrade pip
	$(PY) -m pip install -r requirements.txt

run:
	$(PY) $(SCRIPT)

web-build:
	$(PY) tools/build_web_sim.py $(SIM)
