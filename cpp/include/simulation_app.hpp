#pragma once

#include "simulation.hpp"

#include <string>
#include <vector>

struct SimulationAppOptions {
    std::string initialSimulationId;
    bool showLauncherOnStart = true;
};

int RunSimulationApp(std::vector<SimulationEntry> registry, const SimulationAppOptions& options);
