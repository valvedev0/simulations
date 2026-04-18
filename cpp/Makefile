PLATFORM_OS ?= $(shell uname)

ifeq ($(PLATFORM_OS),Linux)
    CXX := g++
    EXE_EXT :=
    CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
    # Standard linking for Raylib on Linux.
    LDLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
else
    # Windows defaults (w64devkit / standard Raylib install)
    RAYLIB_ROOT ?= C:/raylib/raylib
    RAYLIB_INCLUDE ?= $(RAYLIB_ROOT)/src
    RAYLIB_LIB ?= $(RAYLIB_ROOT)/src/libraylib.a
    TOOLCHAIN_BIN ?= C:/raylib/w64devkit/bin

    CXX := $(TOOLCHAIN_BIN)/g++.exe
    EXE_EXT := .exe
    CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -isystem $(RAYLIB_INCLUDE)
    LDLIBS := $(RAYLIB_LIB) -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++
endif

LAUNCHER_TARGET := build/simulations$(EXE_EXT)
SIM_IDS := flight ants fluid
SIM_TARGETS := $(addprefix build/sims/,$(addsuffix $(EXE_EXT),$(SIM_IDS)))
STANDALONE_OBJECTS := $(addprefix build/obj/standalone/,$(addsuffix .o,$(SIM_IDS)))

SIMULATION_SOURCES := $(wildcard src/simulations/*.cpp)
COMMON_SOURCES := src/simulation_app.cpp src/simulations/registry.cpp $(SIMULATION_SOURCES)
COMMON_SOURCES := $(sort $(COMMON_SOURCES))
COMMON_OBJECTS := $(patsubst %.cpp,build/obj/%.o,$(COMMON_SOURCES))
LAUNCHER_OBJECTS := build/obj/src/main.o $(COMMON_OBJECTS)

.PHONY: all clean list run run-flight run-ants run-fluid
.SECONDARY: $(STANDALONE_OBJECTS)

all: $(LAUNCHER_TARGET) $(SIM_TARGETS)

list:
	@echo Available simulations:
	@for sim in $(SIM_IDS); do echo "  $$sim"; done

run: $(LAUNCHER_TARGET)
	$(LAUNCHER_TARGET)

run-flight: build/sims/flight$(EXE_EXT)
	build/sims/flight$(EXE_EXT)

run-ants: build/sims/ants$(EXE_EXT)
	build/sims/ants$(EXE_EXT)

run-fluid: build/sims/fluid$(EXE_EXT)
	build/sims/fluid$(EXE_EXT)

$(LAUNCHER_TARGET): $(LAUNCHER_OBJECTS)
	@mkdir -p "$(@D)"
	$(CXX) -o $@ $(LAUNCHER_OBJECTS) $(LDLIBS)

build/sims/%$(EXE_EXT): build/obj/standalone/%.o $(COMMON_OBJECTS)
	@mkdir -p "$(@D)"
	$(CXX) -o $@ $^ $(LDLIBS)

build/obj/standalone/%.o: src/standalone_main.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS) -DDEFAULT_SIM_ID=\"$*\" -c $< -o $@

build/obj/%.o: %.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf build
