#include "simulation.hpp"

#include <memory>
#include <vector>

std::unique_ptr<Simulation> CreateFlightSimulation();
std::unique_ptr<Simulation> CreateAntSimulation();

std::vector<SimulationEntry> BuildSimulationRegistry() {
    return {
        {"flight", "Interactive Flight Simulation", "Diagnostics", "Pygame-style flight sim ported to Raylib for direct C++/Python comparison.", &CreateFlightSimulation},
        {"ants", "Ant Foraging Simulation", "Agent Systems", "Ant colony foraging sim with metrics and CSV event records.", &CreateAntSimulation},
    };
}
