#include "simulation_app.hpp"

#include "raylib.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kScreenWidth = 1220;
constexpr int kScreenHeight = 640;
constexpr int kHeaderHeight = 56;
constexpr int kSidebarWidth = 330;

enum class AppMode {
    Launcher,
    Running,
};

struct AppState {
    std::vector<SimulationEntry> registry;
    std::unique_ptr<Simulation> currentSimulation;
    int currentIndex = 0;
    bool quitFromRunning = false;
    AppMode mode = AppMode::Launcher;
};

int FindSimulationIndexById(const std::vector<SimulationEntry>& registry, const std::string& id) {
    const auto iterator = std::find_if(registry.begin(), registry.end(), [&id](const SimulationEntry& entry) {
        return entry.id == id;
    });

    if (iterator == registry.end()) {
        return -1;
    }

    return static_cast<int>(std::distance(registry.begin(), iterator));
}

void StartSimulation(AppState& state, int index) {
    if (state.registry.empty()) {
        return;
    }

    state.currentIndex = std::clamp(index, 0, static_cast<int>(state.registry.size()) - 1);
    state.currentSimulation = state.registry[state.currentIndex].create();
    state.mode = AppMode::Running;
}

void MoveSelection(AppState& state, int delta) {
    if (state.registry.empty()) {
        return;
    }

    const int count = static_cast<int>(state.registry.size());
    state.currentIndex = (state.currentIndex + delta + count) % count;
}

Rectangle GetSimulationButtonBounds(int index) {
    return Rectangle{
        24.0f,
        static_cast<float>(96 + index * 74),
        static_cast<float>(kSidebarWidth - 48),
        56.0f,
    };
}

void DrawTextMuted(const char* text, int x, int y, int fontSize) {
    DrawText(text, x, y, fontSize, Color{169, 181, 199, 255});
}

void DrawHeader(const AppState& state) {
    DrawRectangle(0, 0, GetScreenWidth(), kHeaderHeight, Color{30, 34, 42, 255});
    DrawText("Raylib Simulation Workspace", 22, 16, 22, RAYWHITE);

    if (state.mode == AppMode::Running && state.currentSimulation != nullptr) {
        const char* label = TextFormat("%02d/%02d  %s", state.currentIndex + 1, static_cast<int>(state.registry.size()), state.currentSimulation->name());
        DrawText(label, GetScreenWidth() - MeasureText(label, 20) - 22, 18, 20, Color{190, 202, 220, 255});
    }
}

void DrawLauncher(AppState& state) {
    DrawRectangle(0, kHeaderHeight, kSidebarWidth, GetScreenHeight() - kHeaderHeight, Color{24, 27, 34, 255});
    DrawText("Simulations", 24, 70, 20, RAYWHITE);

    const Vector2 mousePosition = GetMousePosition();

    for (int i = 0; i < static_cast<int>(state.registry.size()); ++i) {
        const SimulationEntry& entry = state.registry[i];
        const Rectangle bounds = GetSimulationButtonBounds(i);
        const bool selected = i == state.currentIndex;
        const bool hovered = CheckCollisionPointRec(mousePosition, bounds);

        Color background = Color{36, 41, 51, 255};
        if (selected) {
            background = Color{54, 83, 119, 255};
        } else if (hovered) {
            background = Color{45, 51, 63, 255};
        }

        DrawRectangleRec(bounds, background);
        DrawRectangleLinesEx(bounds, 1.0f, selected ? Color{119, 176, 231, 255} : Color{63, 70, 84, 255});
        DrawText(entry.name.c_str(), static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 9), 18, RAYWHITE);
        DrawText(entry.category.c_str(), static_cast<int>(bounds.x + 14), static_cast<int>(bounds.y + 33), 14, Color{190, 202, 220, 255});

        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state.currentIndex = i;
            StartSimulation(state, i);
        }
    }

    if (state.registry.empty()) {
        DrawTextMuted("No simulations registered.", 24, 104, 18);
        return;
    }

    const SimulationEntry& selected = state.registry[state.currentIndex];
    const int detailX = kSidebarWidth + 42;
    const int detailY = 94;

    DrawText(selected.name.c_str(), detailX, detailY, 34, RAYWHITE);
    DrawText(selected.id.c_str(), detailX, detailY + 44, 18, Color{119, 176, 231, 255});
    DrawText(selected.description.c_str(), detailX, detailY + 84, 20, Color{210, 217, 226, 255});

    DrawRectangle(detailX, detailY + 148, 170, 44, Color{240, 176, 74, 255});
    DrawText("Start", detailX + 58, detailY + 159, 20, Color{20, 22, 27, 255});

    const Rectangle startButton{static_cast<float>(detailX), static_cast<float>(detailY + 148), 170.0f, 44.0f};
    if (CheckCollisionPointRec(mousePosition, startButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        StartSimulation(state, state.currentIndex);
    }
}

void DrawRunning(AppState& state) {
    if (state.currentSimulation != nullptr) {
        state.currentSimulation->draw();
    }
}

}  // namespace

int RunSimulationApp(std::vector<SimulationEntry> registry, const SimulationAppOptions& options) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Interactive Flight Simulation with Diagnostics");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    AppState state;
    state.registry = std::move(registry);
    state.quitFromRunning = !options.showLauncherOnStart;
    state.mode = options.showLauncherOnStart ? AppMode::Launcher : AppMode::Running;

    if (!options.initialSimulationId.empty()) {
        const int index = FindSimulationIndexById(state.registry, options.initialSimulationId);
        if (index >= 0) {
            StartSimulation(state, index);
        } else {
            state.mode = AppMode::Launcher;
        }
    } else if (!options.showLauncherOnStart && !state.registry.empty()) {
        StartSimulation(state, 0);
    }

    while (!WindowShouldClose()) {
        if (state.mode == AppMode::Launcher) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) {
                MoveSelection(state, 1);
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) {
                MoveSelection(state, -1);
            }
            if (IsKeyPressed(KEY_ENTER) && !state.registry.empty()) {
                StartSimulation(state, state.currentIndex);
            }
        } else {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (state.quitFromRunning) {
                    break;
                }
                state.currentSimulation.reset();
                state.mode = AppMode::Launcher;
            }
            if ((IsKeyPressed(KEY_N) || IsKeyPressed(KEY_RIGHT)) && state.registry.size() > 1) {
                MoveSelection(state, 1);
                StartSimulation(state, state.currentIndex);
            }
            if ((IsKeyPressed(KEY_LEFT)) && state.registry.size() > 1) {
                MoveSelection(state, -1);
                StartSimulation(state, state.currentIndex);
            }
            if (IsKeyPressed(KEY_R) && state.currentSimulation != nullptr) {
                state.currentSimulation->reset();
            }
            if (state.currentSimulation != nullptr) {
                state.currentSimulation->update(GetFrameTime());
            }
        }

        BeginDrawing();
        ClearBackground(Color{20, 22, 27, 255});

        if (state.mode == AppMode::Launcher) {
            DrawLauncher(state);
        } else {
            DrawRunning(state);
        }

        if (state.mode == AppMode::Launcher) {
            DrawHeader(state);
        }
        EndDrawing();
    }

    state.currentSimulation.reset();
    CloseWindow();
    return 0;
}
