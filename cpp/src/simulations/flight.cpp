#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <algorithm>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kUiWidth = 320;
constexpr int kHeight = 640;
constexpr int kStarCount = 200;
constexpr float kMaxSpeed = 400.0f;
constexpr float kAcceleration = 150.0f;
constexpr float kDrag = 0.5f;
constexpr float kTurnSpeedDeg = 180.0f;
constexpr float kPi = 3.14159265f;
constexpr float kTargetRadius = 40.0f;

struct Star {
    Vector2 position;
    float brightness;
    float radius;
};

struct Obstacle {
    Vector2 position;
    float radius;
};

float DegreesToRadians(float degrees) {
    return degrees * (kPi / 180.0f);
}

float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float RandomFloat(float min, float max) {
    return min + (max - min) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

float WrapAngle(float angle) {
    while (angle > kPi) angle -= 2.0f * kPi;
    while (angle < -kPi) angle += 2.0f * kPi;
    return angle;
}

Vector2 Direction(float angle) {
    return {cosf(angle), sinf(angle)};
}

class FlightSimulation : public Simulation {
public:
    FlightSimulation() {
        GenerateStars();
        reset();
    }

    const char* name() const override { return "Space Checkpoint Racer"; }

    void reset() override {
        position_ = { (kSimWidth - kUiWidth) / 2.0f, kHeight / 2.0f };
        velocity_ = { 0, 0 };
        headingDeg_ = -90.0f; // Pointing up
        throttle_ = 0.0f;
        
        level_ = 1;
        score_ = 0;
        timeRemaining_ = 30.0f;
        
        gameState_ = 0; // 0: Playing, 1: Level Complete, 2: Game Over
        
        GenerateLevel();
    }

    void update(float deltaTime) override {
        if (IsKeyPressed(KEY_R)) {
            reset();
            return;
        }

        if (gameState_ == 1) {
            // Level Complete state
            if (IsKeyPressed(KEY_ENTER)) {
                level_++;
                timeRemaining_ += 15.0f; // Bonus time
                GenerateLevel();
                gameState_ = 0;
            }
            return;
        } else if (gameState_ == 2) {
            // Game Over state
            if (IsKeyPressed(KEY_ENTER)) {
                reset();
            }
            return;
        }

        float dt = std::min(deltaTime, 0.05f);
        timeRemaining_ -= dt;
        
        if (timeRemaining_ <= 0) {
            gameState_ = 2; // Time up!
            return;
        }

        ApplyControls(dt);
        Integrate(dt);
        CheckCollisions();
    }

    void draw() const override {
        DrawRectangle(0, 0, kSimWidth - kUiWidth, kHeight, Color{10, 15, 25, 255});
        
        DrawStars();
        DrawObstacles();
        DrawWaypoint();
        DrawAircraft();
        DrawUI();

        if (gameState_ == 1) {
            DrawRectangle(0, 0, kSimWidth, kHeight, Fade(GREEN, 0.3f));
            DrawText("SECTOR CLEARED!", kSimWidth/2 - 200, kHeight/2 - 60, 40, RAYWHITE);
            DrawText(TextFormat("Time Bonus: +15s | Score: %d", score_), kSimWidth/2 - 200, kHeight/2, 20, GOLD);
            DrawText("Press [ENTER] to Continue", kSimWidth/2 - 200, kHeight/2 + 50, 24, RAYWHITE);
        } else if (gameState_ == 2) {
            DrawRectangle(0, 0, kSimWidth, kHeight, Fade(RED, 0.5f));
            DrawText("MISSION FAILED", kSimWidth/2 - 180, kHeight/2 - 60, 40, RAYWHITE);
            if (timeRemaining_ <= 0) {
                DrawText("OUT OF TIME!", kSimWidth/2 - 100, kHeight/2, 24, GOLD);
            } else {
                DrawText("HULL DESTROYED!", kSimWidth/2 - 120, kHeight/2, 24, GOLD);
            }
            DrawText("Press [ENTER] or [R] to Retry", kSimWidth/2 - 200, kHeight/2 + 50, 24, RAYWHITE);
        }
    }

private:
    void GenerateStars() {
        stars_.clear();
        for (int i = 0; i < kStarCount; ++i) {
            stars_.push_back({
                { (float)GetRandomValue(0, kSimWidth - kUiWidth), (float)GetRandomValue(0, kHeight) },
                (float)GetRandomValue(50, 255) / 255.0f,
                (float)GetRandomValue(1, 3)
            });
        }
    }

    void GenerateLevel() {
        checkpoints_.clear();
        obstacles_.clear();

        int numCheckpoints = 3 + level_;
        for (int i = 0; i < numCheckpoints; ++i) {
            Vector2 cp;
            bool valid = false;
            while (!valid) {
                cp = { (float)GetRandomValue(50, kSimWidth - kUiWidth - 50), (float)GetRandomValue(50, kHeight - 50) };
                // Ensure it's not too close to ship initially
                if (i > 0 || Distance(position_, cp) > 150.0f) valid = true;
            }
            checkpoints_.push_back(cp);
        }
        currentCheckpoint_ = 0;

        int numObstacles = level_ * 2;
        for (int i = 0; i < numObstacles; ++i) {
            obstacles_.push_back({
                { (float)GetRandomValue(50, kSimWidth - kUiWidth - 50), (float)GetRandomValue(50, kHeight - 50) },
                (float)GetRandomValue(20, 60)
            });
        }
        
        position_ = { (kSimWidth - kUiWidth) / 2.0f, kHeight / 2.0f };
        velocity_ = { 0, 0 };
    }

    void ApplyControls(float dt) {
        float turnInput = 0.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) turnInput -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) turnInput += 1.0f;

        headingDeg_ += turnInput * kTurnSpeedDeg * dt;
        if (headingDeg_ > 360.0f) headingDeg_ -= 360.0f;
        if (headingDeg_ < 0.0f) headingDeg_ += 360.0f;

        float throttleDelta = 0.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttleDelta += 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttleDelta -= 1.0f;

        throttle_ += throttleDelta * 2.0f * dt;
        throttle_ = std::clamp(throttle_, 0.0f, 1.0f);

        if (IsKeyPressed(KEY_SPACE)) {
            velocity_ = { 0, 0 };
            throttle_ = 0.0f;
        }
    }

    void Integrate(float dt) {
        float headingRad = DegreesToRadians(headingDeg_);
        Vector2 forward = Direction(headingRad);

        Vector2 thrust = Vector2Scale(forward, throttle_ * kAcceleration);
        Vector2 dragForce = Vector2Scale(velocity_, -kDrag);
        Vector2 acceleration = Vector2Add(thrust, dragForce);

        velocity_ = Vector2Add(velocity_, Vector2Scale(acceleration, dt));
        float speed = Vector2Length(velocity_);
        if (speed > kMaxSpeed) {
            velocity_ = Vector2Scale(Vector2Normalize(velocity_), kMaxSpeed);
        }

        position_ = Vector2Add(position_, Vector2Scale(velocity_, dt));

        // Screen wrap
        const float rightEdge = kSimWidth - kUiWidth;
        if (position_.x < 0) position_.x = rightEdge;
        if (position_.x > rightEdge) position_.x = 0;
        if (position_.y < 0) position_.y = kHeight;
        if (position_.y > kHeight) position_.y = 0;
    }

    void CheckCollisions() {
        // Hit Obstacle
        for (const auto& obs : obstacles_) {
            if (Distance(position_, obs.position) < obs.radius + 10.0f) {
                gameState_ = 2; // Crashed
                return;
            }
        }

        // Hit Checkpoint
        if (currentCheckpoint_ < checkpoints_.size()) {
            if (Distance(position_, checkpoints_[currentCheckpoint_]) < kTargetRadius) {
                currentCheckpoint_++;
                score_ += 50 + (int)(timeRemaining_ * 10); // Speed bonus
                timeRemaining_ += 5.0f; // Small time bump per checkpoint
                
                if (currentCheckpoint_ >= checkpoints_.size()) {
                    gameState_ = 1; // Level Complete
                }
            }
        }
    }

    void DrawStars() const {
        for (const auto& star : stars_) {
            // Twinkle effect
            float brightness = star.brightness * (0.8f + 0.2f * sinf(GetTime() * 5.0f + star.position.x));
            DrawCircleV(star.position, star.radius, ColorAlpha(RAYWHITE, brightness));
        }
    }

    void DrawObstacles() const {
        for (const auto& obs : obstacles_) {
            DrawCircleV(obs.position, obs.radius, Color{50, 20, 20, 255});
            DrawCircleLines((int)obs.position.x, (int)obs.position.y, obs.radius, RED);
            DrawText("ASTEROID", (int)obs.position.x - 30, (int)obs.position.y - 10, 10, RED);
        }
    }

    void DrawWaypoint() const {
        if (currentCheckpoint_ < checkpoints_.size()) {
            Vector2 cp = checkpoints_[currentCheckpoint_];
            DrawCircleLines((int)cp.x, (int)cp.y, kTargetRadius, SKYBLUE);
            DrawCircleV(cp, 8.0f, SKYBLUE);
            
            // Radar pulse effect
            float pulse = fmodf(GetTime() * 50.0f, kTargetRadius);
            DrawCircleLines((int)cp.x, (int)cp.y, pulse, ColorAlpha(SKYBLUE, 0.5f));

            DrawText(TextFormat("CP %d/%d", currentCheckpoint_ + 1, (int)checkpoints_.size()), (int)cp.x - 20, (int)cp.y - 60, 14, SKYBLUE);
            
            // Draw a faint line showing the next checkpoint if it exists
            if (currentCheckpoint_ + 1 < checkpoints_.size()) {
                DrawLineEx(cp, checkpoints_[currentCheckpoint_ + 1], 2.0f, Fade(SKYBLUE, 0.2f));
            }
        }
    }

    void DrawAircraft() const {
        float headingRad = DegreesToRadians(headingDeg_);
        Vector2 forward = Direction(headingRad);
        Vector2 right = {-forward.y, forward.x};

        Vector2 nose = Vector2Add(position_, Vector2Scale(forward, 20.0f));
        Vector2 p2 = Vector2Subtract(position_, Vector2Add(Vector2Scale(forward, 10.0f), Vector2Scale(right, 15.0f)));
        Vector2 p3 = Vector2Subtract(position_, Vector2Subtract(Vector2Scale(forward, 10.0f), Vector2Scale(right, 15.0f)));

        DrawTriangle(nose, p2, p3, RAYWHITE);
        
        // Engine Thruster
        if (throttle_ > 0.0f) {
            Vector2 flame = Vector2Subtract(position_, Vector2Scale(forward, 15.0f + (throttle_ * 20.0f * (1.0f + RandomFloat(-0.2f, 0.2f)))));
            DrawTriangle(p2, flame, p3, ORANGE);
            DrawTriangle(Vector2Add(p2, Vector2Scale(right, 5.0f)), flame, Vector2Subtract(p3, Vector2Scale(right, 5.0f)), YELLOW);
        }
    }

    void DrawUI() const {
        const int panelX = kSimWidth - kUiWidth;
        DrawRectangle(panelX, 0, kUiWidth, kHeight, Color{20, 24, 30, 255});
        DrawLine(panelX, 0, panelX, kHeight, Color{60, 70, 80, 255});

        int x = panelX + 20;
        int y = 20;

        DrawText("CHECKPOINT RACER", x, y, 24, RAYWHITE); y += 40;
        DrawText(TextFormat("SECTOR: %d", level_), x, y, 20, GOLD); y += 30;
        DrawText(TextFormat("SCORE: %d", score_), x, y, 20, RAYWHITE); y += 40;

        // Timer
        Color timeColor = timeRemaining_ < 10.0f ? RED : SKYBLUE;
        DrawText("TIME REMAINING", x, y, 14, LIGHTGRAY); y += 20;
        DrawText(TextFormat("%.1f s", timeRemaining_), x, y, 30, timeColor); y += 50;

        // Telemetry
        DrawText("TELEMETRY", x, y, 16, GOLD); y += 25;
        DrawText(TextFormat("Speed: %.0f u/s", Vector2Length(velocity_)), x, y, 16, LIGHTGRAY); y += 20;
        DrawText(TextFormat("Heading: %03.0f deg", headingDeg_), x, y, 16, LIGHTGRAY); y += 20;
        
        // Throttle Bar
        DrawText("Throttle:", x, y, 16, LIGHTGRAY); 
        DrawRectangle(x + 80, y + 2, 100, 12, DARKGRAY);
        DrawRectangle(x + 80, y + 2, (int)(100 * throttle_), 12, ORANGE);
        y += 50;

        // Controls Guide Box
        DrawRectangle(x - 5, y - 5, kUiWidth - 30, 180, Fade(BLACK, 0.5f));
        DrawRectangleLines(x - 5, y - 5, kUiWidth - 30, 180, DARKGRAY);
        
        DrawText("CONTROLS GUIDE", x, y, 16, SKYBLUE); y += 25;
        DrawText("W / S : Throttle Up/Down", x, y, 14, RAYWHITE); y += 22;
        DrawText("A / D : Turn Left/Right", x, y, 14, RAYWHITE); y += 22;
        DrawText("SPACE : E-Brake (Stop)", x, y, 14, RAYWHITE); y += 22;
        DrawText("ENTER : Next Level / Retry", x, y, 14, RAYWHITE); y += 22;
        DrawText("R     : Full Reset", x, y, 14, RAYWHITE);
    }

    Vector2 position_;
    Vector2 velocity_;
    float headingDeg_;
    float throttle_;
    
    int level_;
    int score_;
    float timeRemaining_;
    int gameState_; 
    size_t currentCheckpoint_;

    std::vector<Star> stars_;
    std::vector<Vector2> checkpoints_;
    std::vector<Obstacle> obstacles_;
};

}  // namespace

std::unique_ptr<Simulation> CreateFlightSimulation() {
    return std::make_unique<FlightSimulation>();
}
