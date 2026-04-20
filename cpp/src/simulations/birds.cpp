#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <string>

namespace {

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
};

struct RainDrop {
    Vector2 position;
    float speed;
};

class BirdSimulation : public Simulation {
public:
    BirdSimulation() {
        reset();
    }

    const char* name() const override { return "Bird Flight & Weather"; }

    void reset() override {
        birds.clear();
        for (int i = 0; i < 40; ++i) {
            birds.push_back({
                { (float)GetRandomValue(100, 700), (float)GetRandomValue(100, 500) },
                { (float)GetRandomValue(-50, 50), (float)GetRandomValue(-50, 50) },
                (float)GetRandomValue(0, 100) / 10.0f
            });
        }

        raindrops.clear();
        for (int i = 0; i < 200; ++i) {
            raindrops.push_back({
                { (float)GetRandomValue(0, 1220), (float)GetRandomValue(0, 640) },
                (float)GetRandomValue(400, 700)
            });
        }

        mode = WeatherMode::Clear;
        simulationSpeed = 1.0f;
    }

    void update(float deltaTime) override {
        // Handle input
        if (IsKeyPressed(KEY_ONE)) mode = WeatherMode::Clear;
        if (IsKeyPressed(KEY_TWO)) mode = WeatherMode::LightWind;
        if (IsKeyPressed(KEY_THREE)) mode = WeatherMode::HeavyWind;
        if (IsKeyPressed(KEY_FOUR)) mode = WeatherMode::Rain;

        if (IsKeyDown(KEY_EQUAL)) simulationSpeed += deltaTime * 2.0f;
        if (IsKeyDown(KEY_MINUS)) simulationSpeed -= deltaTime * 2.0f;
        if (simulationSpeed < 0.1f) simulationSpeed = 0.1f;
        if (simulationSpeed > 5.0f) simulationSpeed = 5.0f;

        Vector2 wind = { 0, 0 };
        if (mode == WeatherMode::LightWind) wind = { 40.0f, 10.0f };
        if (mode == WeatherMode::HeavyWind) wind = { 150.0f, 30.0f };
        if (mode == WeatherMode::Rain) wind = { 20.0f, 5.0f };

        float dt = deltaTime * simulationSpeed;

        // Update birds
        for (auto& bird : birds) {
            bird.phase += dt * 5.0f;
            
            // Basic flocking/wandering
            Vector2 noise = { (float)sin(bird.phase) * 10.0f, (float)cos(bird.phase * 0.8f) * 10.0f };
            bird.velocity = Vector2Add(bird.velocity, Vector2Scale(noise, dt));
            bird.velocity = Vector2Add(bird.velocity, Vector2Scale(wind, dt));

            // Cap velocity
            float maxVel = (mode == WeatherMode::HeavyWind) ? 300.0f : 150.0f;
            if (Vector2Length(bird.velocity) > maxVel) {
                bird.velocity = Vector2Scale(Vector2Normalize(bird.velocity), maxVel);
            }

            bird.position = Vector2Add(bird.position, Vector2Scale(bird.velocity, dt));

            // Wrap around screen
            if (bird.position.x < 0) bird.position.x = 1220;
            if (bird.position.x > 1220) bird.position.x = 0;
            if (bird.position.y < 0) bird.position.y = 640;
            if (bird.position.y > 640) bird.position.y = 0;
        }

        // Update rain
        if (mode == WeatherMode::Rain) {
            for (auto& drop : raindrops) {
                drop.position.y += drop.speed * dt;
                drop.position.x += wind.x * dt;
                if (drop.position.y > 640) {
                    drop.position.y = -10;
                    drop.position.x = (float)GetRandomValue(0, 1220);
                }
                if (drop.position.x > 1220) drop.position.x = 0;
            }
        }
    }

    void draw() const override {
        // Draw Background Info
        const char* modeText = "Mode: Clear (Press 1-4 to change)";
        if (mode == WeatherMode::LightWind) modeText = "Mode: Light Wind (Press 1-4 to change)";
        if (mode == WeatherMode::HeavyWind) modeText = "Mode: Heavy Wind (Press 1-4 to change)";
        if (mode == WeatherMode::Rain) modeText = "Mode: Rain (Press 1-4 to change)";

        DrawText(modeText, 20, 80, 20, RAYWHITE);
        DrawText(TextFormat("Speed: %.1fx (Press +/-)", simulationSpeed), 20, 110, 20, LIGHTGRAY);

        // Draw Rain
        if (mode == WeatherMode::Rain) {
            for (const auto& drop : raindrops) {
                DrawLine(drop.position.x, drop.position.y, drop.position.x + 2, drop.position.y + 10, Fade(BLUE, 0.5f));
            }
        }

        // Draw Birds
        for (const auto& bird : birds) {
            float rotation = atan2f(bird.velocity.y, bird.velocity.x) * RAD2DEG;
            float wingFlap = sinf(bird.phase * 2.0f) * 15.0f;

            // Bird Body (Triangle)
            DrawPoly(bird.position, 3, 12.0f, rotation, SKYBLUE);
            
            // Wings
            Vector2 wingL = { bird.position.x + cosf((rotation - 90 + wingFlap) * DEG2RAD) * 15, bird.position.y + sinf((rotation - 90 + wingFlap) * DEG2RAD) * 15 };
            Vector2 wingR = { bird.position.x + cosf((rotation + 90 - wingFlap) * DEG2RAD) * 15, bird.position.y + sinf((rotation + 90 - wingFlap) * DEG2RAD) * 15 };
            DrawLineEx(bird.position, wingL, 2.0f, RAYWHITE);
            DrawLineEx(bird.position, wingR, 2.0f, RAYWHITE);
        }

        // Wind visualization
        if (mode == WeatherMode::HeavyWind || mode == WeatherMode::LightWind) {
            float intensity = (mode == WeatherMode::HeavyWind) ? 1.0f : 0.4f;
            for (int i = 0; i < 5; ++i) {
                float y = 150 + i * 100;
                float xOffset = fmodf(GetTime() * 500 * intensity, 400);
                for (int j = 0; j < 4; ++j) {
                    DrawLineEx({-200 + j * 400 + xOffset, y}, {-100 + j * 400 + xOffset, y}, 1.0f, Fade(GRAY, 0.3f * intensity));
                }
            }
        }
    }

private:
    std::vector<Bird> birds;
    std::vector<RainDrop> raindrops;
    WeatherMode mode;
    float simulationSpeed;
};

} // namespace

std::unique_ptr<Simulation> CreateBirdSimulation() {
    return std::make_unique<BirdSimulation>();
}
