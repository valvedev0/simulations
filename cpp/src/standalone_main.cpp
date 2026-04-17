#include "simulation_app.hpp"

#ifndef DEFAULT_SIM_ID
#define DEFAULT_SIM_ID ""
#endif

int main() {
    SimulationAppOptions options;
    options.initialSimulationId = DEFAULT_SIM_ID;
    options.showLauncherOnStart = false;

    return RunSimulationApp(BuildSimulationRegistry(), options);
}
