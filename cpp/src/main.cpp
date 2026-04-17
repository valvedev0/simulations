#include "simulation_app.hpp"

#include <string>

int main(int argc, char** argv) {
    SimulationAppOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--menu") {
            options.showLauncherOnStart = true;
            options.initialSimulationId.clear();
        } else if (argument == "--sim" && i + 1 < argc) {
            options.initialSimulationId = argv[++i];
            options.showLauncherOnStart = false;
        } else if (argument.rfind("--sim=", 0) == 0) {
            options.initialSimulationId = argument.substr(6);
            options.showLauncherOnStart = false;
        } else if (!argument.empty() && argument[0] != '-') {
            options.initialSimulationId = argument;
            options.showLauncherOnStart = false;
        }
    }

    return RunSimulationApp(BuildSimulationRegistry(), options);
}
