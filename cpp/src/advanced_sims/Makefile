# Makefile for Advanced Compute Shader Particle Simulation (Linux - GTX 1060)
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -I../../include -I/usr/local/include -I/usr/include
LDLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET := ../../build/advanced/gpu_particles

all: $(TARGET)

$(TARGET): gpu_particles.cpp
	@mkdir -p ../../build/advanced
	@echo "Compiling Compute Shader Particle Sim for Linux (GTX 1060)..."
	$(CXX) $(CXXFLAGS) gpu_particles.cpp -o $(TARGET) $(LDLIBS)
	@echo "Build successful! Run the simulation with: ./$(TARGET)"

clean:
	rm -f $(TARGET)
