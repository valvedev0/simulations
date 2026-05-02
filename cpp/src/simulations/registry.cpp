#include "simulation.hpp"

#include <memory>
#include <vector>

std::unique_ptr<Simulation> CreateFlightSimulation();
std::unique_ptr<Simulation> CreateAntSimulation();
std::unique_ptr<Simulation> CreateFluidSimulation();
std::unique_ptr<Simulation> CreateBirdSimulation();
std::unique_ptr<Simulation> CreateRocketSimulation();
std::unique_ptr<Simulation> CreateGalaxySimulation();
std::unique_ptr<Simulation> CreateSynthSimulation();
std::unique_ptr<Simulation> CreateRacingSimulation();

std::vector<SimulationEntry> BuildSimulationRegistry() {
    return {
        {"flight", "Interactive Flight Simulation", "Diagnostics", "Pygame-style flight sim ported to Raylib for direct C++/Python comparison.", &CreateFlightSimulation},
        {"ants", "Ant Foraging Simulation", "Agent Systems", "Ant colony foraging sim with metrics and CSV event records.", &CreateAntSimulation},
        {"fluid", "Particle Fluid Simulation", "Physics", "Interactive particle-based fluid simulation that reacts to mouse clicks.", &CreateFluidSimulation},
        {"birds", "Bird Flight & Weather", "Nature", "Birds flying in various weather conditions controlled by keys.", &CreateBirdSimulation},
        {"racing", "3D Open World Racing", "Games", "A lightweight 3D racing game featuring player vs AI in a generated open world.", &CreateRacingSimulation},
        {"rocket", "Rocket Landing PID", "Aerospace", "2D rocket landing simulation with PID-controlled thrust and orientation.", &CreateRocketSimulation},
        {"galaxy", "GPU Galaxy Sim", "High Performance", "Renders 50,000+ interacting particles using optimized rlgl batching.", &CreateGalaxySimulation},
        {"synth", "Retro Synthesizer", "Audio", "Real-time audio synthesizer and sequencer with piano keyboard.", &CreateSynthSimulation},
    };
}
