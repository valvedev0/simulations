RAYLIB_ROOT ?= C:/raylib/raylib
RAYLIB_INCLUDE ?= $(RAYLIB_ROOT)/src
RAYLIB_LIB ?= $(RAYLIB_ROOT)/src/libraylib.a
TOOLCHAIN_BIN ?= C:/raylib/w64devkit/bin

CXX := $(TOOLCHAIN_BIN)/g++.exe
LAUNCHER_TARGET := build/simulations.exe
SIM_IDS := flight ants
SIM_TARGETS := $(addprefix build/sims/,$(addsuffix .exe,$(SIM_IDS)))
STANDALONE_OBJECTS := $(addprefix build/obj/standalone/,$(addsuffix .o,$(SIM_IDS)))

SIMULATION_SOURCES := $(wildcard src/simulations/*.cpp)
COMMON_SOURCES := src/simulation_app.cpp src/simulations/registry.cpp $(SIMULATION_SOURCES)
COMMON_SOURCES := $(sort $(COMMON_SOURCES))
COMMON_OBJECTS := $(patsubst %.cpp,build/obj/%.o,$(COMMON_SOURCES))
LAUNCHER_OBJECTS := build/obj/src/main.o $(COMMON_OBJECTS)

CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -isystem $(RAYLIB_INCLUDE)
LDLIBS := $(RAYLIB_LIB) -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++

.PHONY: all clean list run run-flight run-ants
.SECONDARY: $(STANDALONE_OBJECTS)

all: $(LAUNCHER_TARGET) $(SIM_TARGETS)

list:
	@echo Available simulations:
	@for sim in $(SIM_IDS); do echo "  $$sim"; done

run: $(LAUNCHER_TARGET)
	$(LAUNCHER_TARGET)

run-flight: build/sims/flight.exe
	build/sims/flight.exe

run-ants: build/sims/ants.exe
	build/sims/ants.exe

$(LAUNCHER_TARGET): $(LAUNCHER_OBJECTS)
	@mkdir -p "$(@D)"
	$(CXX) -o $@ $(LAUNCHER_OBJECTS) $(LDLIBS)

build/sims/%.exe: build/obj/standalone/%.o $(COMMON_OBJECTS)
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
