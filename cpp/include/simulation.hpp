#pragma once

#include <memory>
#include <string>
#include <vector>

class Simulation {
public:
    virtual ~Simulation() = default;

    virtual const char* name() const = 0;
    virtual void reset() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void draw() const = 0;
};

using SimulationFactory = std::unique_ptr<Simulation> (*)();

struct SimulationEntry {
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    SimulationFactory create;
};

std::vector<SimulationEntry> BuildSimulationRegistry();
