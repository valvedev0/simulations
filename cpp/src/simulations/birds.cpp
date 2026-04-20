#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <string>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kSimHeight = 640;

enum class WeatherMode {
    Clear,
    LightWind,
    HeavyWind,
    Rain
};

struct Bird {
    Vector2 position;
    Vector2 velocity;
    float phase;
    bool isPanicking = false;
};

struct RainDrop {
    Vector2 position;
    float speed;
};

struct Obstacle {
    Vector2 position;
    float radius;
    Color color;
};

class BirdSimulation : public Simulation {
public:
    BirdSimulation() {
        reset();
    }

    const char* name() const override { return "Interactive Bird Flock & Predator"; }

    void reset() override {
        birds.clear();
        for (int i = 0; i < 60; ++i) {
            birds.push_back({
                { (float)GetRandomValue(100, 700), (float)GetRandomValue(100, 500) },
                { (float)GetRandomValue(-50, 50), (float)GetRandomValue(-50, 50) },
                (float)GetRandomValue(0, 100) / 10.0f,
                false
            });
        }

        raindrops.clear();
        for (int i = 0; i < 300; ++i) {
            raindrops.push_back({
                { (float)GetRandomValue(0, kSimWidth), (float)GetRandomValue(0, kSimHeight) },
                (float)GetRandomValue(400, 700)
            });
        }
        
        obstacles.clear();
        for (int i = 0; i < 4; ++i) {
            obstacles.push_back({
                { (float)GetRandomValue(200, kSimWidth - 200), (float)GetRandomValue(100, kSimHeight - 100) },
                (float)GetRandomValue(30, 60),
                DARKGRAY
            });
        }

        mode = WeatherMode::Clear;
        simulationSpeed = 1.0f;
        windDirection = { 1.0f, 0.0f }; // Default pointing right
        targetWaypoint = { kSimWidth / 2.0f, kSimHeight / 2.0f };
        hasWaypoint = false;
        
        predatorActive = false;
        predatorPos = { -100, -100 };
    }

    void update(float deltaTime) override {
        // --- INPUT HANDLING ---
        if (IsKeyPressed(KEY_ONE)) mode = WeatherMode::Clear;
        if (IsKeyPressed(KEY_TWO)) mode = WeatherMode::LightWind;
        if (IsKeyPressed(KEY_THREE)) mode = WeatherMode::HeavyWind;
        if (IsKeyPressed(KEY_FOUR)) mode = WeatherMode::Rain;

        if (IsKeyDown(KEY_EQUAL)) simulationSpeed += deltaTime * 2.0f;
        if (IsKeyDown(KEY_MINUS)) simulationSpeed -= deltaTime * 2.0f;
        if (simulationSpeed < 0.1f) simulationSpeed = 0.1f;
        if (simulationSpeed > 5.0f) simulationSpeed = 5.0f;

        Vector2 mousePos = GetMousePosition();

        // Left Click: Set Flock Target Waypoint
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            targetWaypoint = mousePos;
            hasWaypoint = true;
        }

        // Right Click: Set Wind Direction
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            Vector2 center = { kSimWidth / 2.0f, kSimHeight / 2.0f };
            Vector2 dir = Vector2Subtract(mousePos, center);
            if (Vector2Length(dir) > 1.0f) {
                windDirection = Vector2Normalize(dir);
            }
        }

        // Spacebar: Spawn/Move Predator
        if (IsKeyPressed(KEY_SPACE)) {
            predatorActive = true;
            predatorPos = mousePos;
        }

        // --- PHYSICS & WEATHER ---
        float windMagnitude = 0.0f;
        if (mode == WeatherMode::LightWind) windMagnitude = 40.0f;
        if (mode == WeatherMode::HeavyWind) windMagnitude = 150.0f;
        if (mode == WeatherMode::Rain) windMagnitude = 20.0f;
        
        Vector2 wind = Vector2Scale(windDirection, windMagnitude);
        float dt = deltaTime * simulationSpeed;

        // --- PREDATOR LOGIC ---
        if (predatorActive) {
            // Predator chases the closest bird
            int closestIdx = -1;
            float minDist = 99999.0f;
            for (int i = 0; i < (int)birds.size(); ++i) {
                float d = Vector2Distance(predatorPos, birds[i].position);
                if (d < minDist) { minDist = d; closestIdx = i; }
            }
            
            if (closestIdx >= 0) {
                Vector2 dir = Vector2Normalize(Vector2Subtract(birds[closestIdx].position, predatorPos));
                predatorPos = Vector2Add(predatorPos, Vector2Scale(dir, 200.0f * dt));
                
                // Eat bird if caught
                if (minDist < 15.0f) {
                    birds.erase(birds.begin() + closestIdx);
                }
            }
        }

        // --- BIRD LOGIC ---
        for (auto& bird : birds) {
            bird.phase += dt * 5.0f;
            bird.isPanicking = false;
            
            Vector2 acceleration = { 0, 0 };

            // 1. Seek Waypoint
            if (hasWaypoint) {
                Vector2 desired = Vector2Subtract(targetWaypoint, bird.position);
                float d = Vector2Length(desired);
                if (d > 0) {
                    desired = Vector2Scale(Vector2Normalize(desired), 150.0f); // Max speed towards target
                    Vector2 steer = Vector2Subtract(desired, bird.velocity);
                    acceleration = Vector2Add(acceleration, Vector2Scale(steer, 2.0f)); // Steering force
                }
            } else {
                // Basic wandering if no waypoint
                Vector2 noise = { (float)sin(bird.phase) * 10.0f, (float)cos(bird.phase * 0.8f) * 10.0f };
                acceleration = Vector2Add(acceleration, noise);
            }

            // 2. Wind Force
            acceleration = Vector2Add(acceleration, wind);

            // 3. Obstacle Avoidance (Blockers)
            for (const auto& obs : obstacles) {
                Vector2 diff = Vector2Subtract(bird.position, obs.position);
                float d = Vector2Length(diff);
                float avoidRadius = obs.radius + 40.0f; // buffer
                if (d > 0 && d < avoidRadius) {
                    Vector2 avoidForce = Vector2Scale(Vector2Normalize(diff), (avoidRadius - d) * 10.0f);
                    acceleration = Vector2Add(acceleration, avoidForce);
                }
            }

            // 4. Predator Fleeing (Panic)
            if (predatorActive) {
                Vector2 diff = Vector2Subtract(bird.position, predatorPos);
                float d = Vector2Length(diff);
                if (d > 0 && d < 200.0f) { // Panic radius
                    bird.isPanicking = true;
                    Vector2 fleeForce = Vector2Scale(Vector2Normalize(diff), (200.0f - d) * 5.0f);
                    acceleration = Vector2Add(acceleration, fleeForce);
                }
            }

            // Apply acceleration
            bird.velocity = Vector2Add(bird.velocity, Vector2Scale(acceleration, dt));

            // Cap velocity
            float maxVel = bird.isPanicking ? 400.0f : ((mode == WeatherMode::HeavyWind) ? 250.0f : 150.0f);
            if (Vector2Length(bird.velocity) > maxVel) {
                bird.velocity = Vector2Scale(Vector2Normalize(bird.velocity), maxVel);
            }

            // Move
            bird.position = Vector2Add(bird.position, Vector2Scale(bird.velocity, dt));

            // Wrap around screen
            if (bird.position.x < 0) bird.position.x = kSimWidth;
            if (bird.position.x > kSimWidth) bird.position.x = 0;
            if (bird.position.y < 0) bird.position.y = kSimHeight;
            if (bird.position.y > kSimHeight) bird.position.y = 0;
        }

        // --- RAIN LOGIC ---
        if (mode == WeatherMode::Rain) {
            for (auto& drop : raindrops) {
                drop.position.y += drop.speed * dt;
                drop.position.x += wind.x * dt;
                if (drop.position.y > kSimHeight) {
                    drop.position.y = -10;
                    drop.position.x = (float)GetRandomValue(0, kSimWidth);
                }
                if (drop.position.x > kSimWidth) drop.position.x = 0;
                if (drop.position.x < 0) drop.position.x = kSimWidth;
            }
        }
    }

    void draw() const override {
        // --- UI & DIAGNOSTICS ---
        const char* modeText = "Weather: Clear";
        if (mode == WeatherMode::LightWind) modeText = "Weather: Light Wind";
        if (mode == WeatherMode::HeavyWind) modeText = "Weather: Heavy Wind";
        if (mode == WeatherMode::Rain) modeText = "Weather: Rain";

        DrawRectangle(10, 10, 320, 160, Fade(BLACK, 0.7f));
        DrawText(modeText, 20, 20, 18, RAYWHITE);
        DrawText(TextFormat("Speed: %.1fx (Press +/-)", simulationSpeed), 20, 45, 18, LIGHTGRAY);
        DrawText(TextFormat("Birds Alive: %d", (int)birds.size()), 20, 70, 18, GREEN);
        
        DrawText("Controls:", 20, 100, 16, RAYWHITE);
        DrawText("LMB: Set target path", 20, 120, 14, LIGHTGRAY);
        DrawText("RMB: Set wind direction", 20, 140, 14, LIGHTGRAY);
        DrawText("SPACE: Spawn Predator (Hawk)", 170, 120, 14, ORANGE);
        DrawText("1-4: Change Weather", 170, 140, 14, LIGHTGRAY);

        // Draw Wind Direction Indicator (Top Right)
        Vector2 windCenter = { kSimWidth - 60.0f, 60.0f };
        DrawCircleV(windCenter, 40.0f, Fade(BLACK, 0.5f));
        DrawLineEx(windCenter, Vector2Add(windCenter, Vector2Scale(windDirection, 35.0f)), 3.0f, BLUE);
        DrawCircleV(windCenter, 4.0f, RAYWHITE);
        DrawText("WIND", (int)windCenter.x - 18, (int)windCenter.y + 45, 14, RAYWHITE);

        // --- SCENE RENDERING ---
        
        // Draw Obstacles (Blockers)
        for (const auto& obs : obstacles) {
            DrawCircleV(obs.position, obs.radius, obs.color);
            DrawCircleLines((int)obs.position.x, (int)obs.position.y, obs.radius, BLACK);
        }

        // Draw Waypoint
        if (hasWaypoint) {
            DrawCircleV(targetWaypoint, 6.0f, YELLOW);
            DrawCircleLines((int)targetWaypoint.x, (int)targetWaypoint.y, 12.0f, ORANGE);
            DrawCircleLines((int)targetWaypoint.x, (int)targetWaypoint.y, 18.0f, Fade(ORANGE, 0.5f));
        }

        // Draw Rain
        if (mode == WeatherMode::Rain) {
            for (const auto& drop : raindrops) {
                float dropAngle = atan2f(drop.speed, windDirection.x * 20.0f); // fake angle for visual
                Vector2 tail = { drop.position.x + cosf(dropAngle)*2.0f, drop.position.y + sinf(dropAngle)*10.0f };
                DrawLineV(drop.position, tail, Fade(BLUE, 0.5f));
            }
        }

        // Draw Predator
        if (predatorActive) {
            DrawCircleV(predatorPos, 12.0f, RED);
            DrawPoly(predatorPos, 3, 20.0f, GetTime() * 100.0f, MAROON); // Spinning blades effect
            DrawText("PREDATOR!", (int)predatorPos.x - 30, (int)predatorPos.y - 25, 14, RED);
        }

        // Draw Birds
        for (const auto& bird : birds) {
            float rotation = atan2f(bird.velocity.y, bird.velocity.x) * RAD2DEG;
            float wingFlap = sinf(bird.phase * (bird.isPanicking ? 4.0f : 2.0f)) * (bird.isPanicking ? 25.0f : 15.0f);
            
            Color birdColor = bird.isPanicking ? ORANGE : SKYBLUE;

            // Bird Body
            DrawPoly(bird.position, 3, 12.0f, rotation, birdColor);
            
            // Wings
            Vector2 wingL = { bird.position.x + cosf((rotation - 90 + wingFlap) * DEG2RAD) * 15, bird.position.y + sinf((rotation - 90 + wingFlap) * DEG2RAD) * 15 };
            Vector2 wingR = { bird.position.x + cosf((rotation + 90 - wingFlap) * DEG2RAD) * 15, bird.position.y + sinf((rotation + 90 - wingFlap) * DEG2RAD) * 15 };
            DrawLineEx(bird.position, wingL, 2.0f, RAYWHITE);
            DrawLineEx(bird.position, wingR, 2.0f, RAYWHITE);
        }

        // Visual Wind streaks
        if (mode == WeatherMode::HeavyWind || mode == WeatherMode::LightWind) {
            float intensity = (mode == WeatherMode::HeavyWind) ? 1.0f : 0.4f;
            for (int i = 0; i < 6; ++i) {
                Vector2 startPos = { (float)fmodf(GetTime() * 500 * intensity + i * 200, kSimWidth), 100.0f + i * 80.0f };
                Vector2 endPos = Vector2Add(startPos, Vector2Scale(windDirection, 100.0f * intensity));
                DrawLineEx(startPos, endPos, 2.0f, Fade(LIGHTGRAY, 0.2f * intensity));
            }
        }
    }

private:
    std::vector<Bird> birds;
    std::vector<RainDrop> raindrops;
    std::vector<Obstacle> obstacles;
    
    WeatherMode mode;
    float simulationSpeed;
    
    Vector2 windDirection;
    Vector2 targetWaypoint;
    bool hasWaypoint;

    bool predatorActive;
    Vector2 predatorPos;
};

} // namespace

std::unique_ptr<Simulation> CreateBirdSimulation() {
    return std::make_unique<BirdSimulation>();
}
