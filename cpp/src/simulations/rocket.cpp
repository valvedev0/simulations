#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kSimHeight = 640;
constexpr float kGroundY = 580.0f;
constexpr float kPixelsPerMeter = 20.0f;

struct PID {
    float kp, ki, kd;
    float integral = 0;
    float prevError = 0;

    float update(float error, float dt) {
        if (dt <= 0) return 0;
        integral += error * dt;
        // Anti-windup: clamp integral
        integral = std::clamp(integral, -100.0f, 100.0f);
        float derivative = (error - prevError) / dt;
        prevError = error;
        return kp * error + ki * integral + kd * derivative;
    }

    void reset() {
        integral = 0;
        prevError = 0;
    }
};

class RocketSimulation : public Simulation {
public:
    RocketSimulation() {
        reset();
    }

    const char* name() const override { return "Rocket Vertical Landing PID"; }

    void reset() override {
        position = { kSimWidth / 2.0f, 80.0f };
        velocity = { (float)GetRandomValue(-50, 50), 0 };
        angle = (float)GetRandomValue(-20, 20) * DEG2RAD;
        angularVelocity = 0;
        mass = 1.0f;
        
        fuel = 100.0f;
        score = 0;
        maxFuel = 100.0f;
        
        landingPadX = (float)GetRandomValue(200, kSimWidth - 200);
        landingPadWidth = 100.0f;

        gravity = 9.81f * kPixelsPerMeter;
        maxThrust = gravity * 2.5f;
        maxTorque = 6.0f;
        dragCoeff = 0.15f;

        velPID = { 1.8f, 0.4f, 1.2f };
        anglePID = { 14.0f, 0.2f, 10.0f };
        
        targetDescentSpeed = 1.5f * kPixelsPerMeter;
        isLanded = false;
        isCrashed = false;
        autoPilot = false;
        thrustLevel = 0;
    }

    void update(float deltaTime) override {
        if (IsKeyPressed(KEY_R)) reset();
        if (IsKeyPressed(KEY_P)) autoPilot = !autoPilot;

        if (isLanded || isCrashed) return;

        float dt = deltaTime;
        if (dt > 0.1f) dt = 0.1f;

        // --- CONTROL SYSTEM ---
        float torqueReq = 0;
        float thrustReq = 0;

        if (fuel > 0) {
            if (autoPilot) {
                float angleError = 0 - angle;
                torqueReq = anglePID.update(angleError, dt);

                float velError = targetDescentSpeed - velocity.y;
                thrustReq = velPID.update(velError, dt);
                thrustReq += gravity * mass;
            } else {
                if (IsKeyDown(KEY_W)) thrustReq = maxThrust;
                if (IsKeyDown(KEY_A)) torqueReq = -maxTorque;
                if (IsKeyDown(KEY_D)) torqueReq = maxTorque;
            }
        }

        thrustLevel = std::clamp(thrustReq, 0.0f, maxThrust);
        if (fuel <= 0) thrustLevel = 0;
        
        float torque = std::clamp(torqueReq, -maxTorque, maxTorque);

        // Consume Fuel
        if (thrustLevel > 0) {
            fuel -= (thrustLevel / maxThrust) * 10.0f * dt;
            if (fuel < 0) fuel = 0;
        }

        // --- PHYSICS ---
        Vector2 force = { 0, gravity * mass };
        Vector2 thrustDir = { sinf(angle), -cosf(angle) };
        force = Vector2Add(force, Vector2Scale(thrustDir, thrustLevel));

        Vector2 drag = Vector2Scale(velocity, -dragCoeff);
        force = Vector2Add(force, drag);

        Vector2 acceleration = Vector2Scale(force, 1.0f / mass);
        velocity = Vector2Add(velocity, Vector2Scale(acceleration, dt));
        position = Vector2Add(position, Vector2Scale(velocity, dt));

        float angularAcceleration = torque / mass;
        angularVelocity += angularAcceleration * dt;
        angle += angularVelocity * dt;

        // Boundary Constraints
        if (position.x < 20) {
            position.x = 20;
            velocity.x *= -0.5f; // Bounce
        }
        if (position.x > kSimWidth - 20) {
            position.x = kSimWidth - 20;
            velocity.x *= -0.5f;
        }
        if (position.y < 20) {
            position.y = 20;
            velocity.y = std::max(0.0f, velocity.y);
        }

        // Collision Check
        if (position.y >= kGroundY) {
            position.y = kGroundY;
            
            float impactSpeed = Vector2Length(velocity);
            float impactAngle = fabsf(angle * RAD2DEG);
            bool onPad = (position.x >= landingPadX - landingPadWidth/2) && 
                         (position.x <= landingPadX + landingPadWidth/2);

            if (impactSpeed < 110.0f && impactAngle < 15.0f && onPad) {
                isLanded = true;
                score = (int)(fuel * 10.0f + (110.0f - impactSpeed) * 5.0f);
            } else {
                isCrashed = true;
            }
            velocity = { 0, 0 };
            angularVelocity = 0;
            thrustLevel = 0;
        }
    }

    void draw() const override {
        // Draw Ground
        DrawRectangle(0, (int)kGroundY, kSimWidth, kSimHeight - (int)kGroundY, Color{30, 30, 35, 255});
        
        // Draw Landing Pad
        DrawRectangle((int)(landingPadX - landingPadWidth/2), (int)kGroundY - 4, (int)landingPadWidth, 8, SKYBLUE);
        for (int i = 0; i < 3; ++i) {
            DrawRectangleLines((int)(landingPadX - landingPadWidth/2) - i, (int)kGroundY - 4 - i, (int)landingPadWidth + i*2, 8 + i*2, Fade(SKYBLUE, 0.5f));
        }
        DrawText("TARGET PAD", (int)landingPadX - 40, (int)kGroundY + 10, 14, SKYBLUE);

        // Draw Rocket
        float rocketWidth = 14.0f;
        Rectangle rocketRect = { position.x, position.y, rocketWidth, 40.0f };
        Vector2 origin = { rocketWidth / 2.0f, 40.0f };
        
        // Shadow
        DrawEllipse(position.x, kGroundY, 20 * (position.y / kGroundY), 5, Fade(BLACK, 0.3f));

        if (!isCrashed) {
            DrawRectanglePro(rocketRect, origin, angle * RAD2DEG, RAYWHITE);
            
            // Nose
            Vector2 p1 = Vector2Add(position, Vector2Rotate({-rocketWidth/2, -40}, angle * RAD2DEG));
            Vector2 p2 = Vector2Add(position, Vector2Rotate({rocketWidth/2, -40}, angle * RAD2DEG));
            Vector2 p3 = Vector2Add(position, Vector2Rotate({0, -55}, angle * RAD2DEG));
            DrawTriangle(p1, p2, p3, RED);

            // Legs
            Vector2 l1 = Vector2Add(position, Vector2Rotate({-rocketWidth/2, 0}, angle * RAD2DEG));
            Vector2 l2 = Vector2Add(position, Vector2Rotate({-rocketWidth/2 - 8, 10}, angle * RAD2DEG));
            Vector2 r1 = Vector2Add(position, Vector2Rotate({rocketWidth/2, 0}, angle * RAD2DEG));
            Vector2 r2 = Vector2Add(position, Vector2Rotate({rocketWidth/2 + 8, 10}, angle * RAD2DEG));
            DrawLineEx(l1, l2, 3.0f, GRAY);
            DrawLineEx(r1, r2, 3.0f, GRAY);

            // Flame
            if (thrustLevel > 1.0f && fuel > 0) {
                float flameScale = (thrustLevel / maxThrust) * 40.0f;
                Vector2 f1 = Vector2Add(position, Vector2Rotate({-rocketWidth/3, 2}, angle * RAD2DEG));
                Vector2 f2 = Vector2Add(position, Vector2Rotate({rocketWidth/3, 2}, angle * RAD2DEG));
                Vector2 f3 = Vector2Add(position, Vector2Rotate({0, flameScale + (float)GetRandomValue(5, 15)}, angle * RAD2DEG));
                DrawTriangle(f1, f3, f2, ORANGE);
                DrawTriangle(f1, Vector2Add(f3, {0, -5}), f2, YELLOW);
            }
        } else {
            // Explosion particles / scrap
            DrawCircleV(position, 20, RED);
            DrawCircleV(position, 15, ORANGE);
            DrawCircleV(position, 10, YELLOW);
            DrawText("KABOOM!", position.x - 40, position.y - 40, 20, RED);
        }

        // --- UI ---
        // Left Panel: Stats
        DrawRectangle(10, 10, 240, 160, Fade(BLACK, 0.8f));
        DrawText("ROCKET LANDER", 20, 20, 20, RAYWHITE);
        
        // Fuel Bar
        DrawText("FUEL", 20, 50, 14, RAYWHITE);
        DrawRectangle(70, 52, 150, 12, DARKGRAY);
        DrawRectangle(70, 52, (int)(150 * (fuel / maxFuel)), 12, fuel > 20 ? GREEN : RED);
        
        DrawText(TextFormat("ALTITUDE: %d m", (int)((kGroundY - position.y) / kPixelsPerMeter)), 20, 75, 16, LIGHTGRAY);
        DrawText(TextFormat("V-SPEED: %.1f m/s", velocity.y / kPixelsPerMeter), 20, 95, 16, velocity.y > 110.0f ? RED : GREEN);
        DrawText(TextFormat("H-SPEED: %.1f m/s", velocity.x / kPixelsPerMeter), 20, 115, 16, LIGHTGRAY);
        DrawText(TextFormat("AUTOPILOT: %s", autoPilot ? "ON" : "OFF"), 20, 140, 16, autoPilot ? SKYBLUE : ORANGE);

        // Right Panel: Controls Guide
        DrawRectangle(kSimWidth - 210, 10, 200, 140, Fade(BLACK, 0.8f));
        DrawText("CONTROLS", kSimWidth - 200, 20, 16, SKYBLUE);
        DrawText("W: Main Engine", kSimWidth - 200, 45, 14, RAYWHITE);
        DrawText("A/D: RCS Thrusters", kSimWidth - 200, 65, 14, RAYWHITE);
        DrawText("P: Toggle Autopilot", kSimWidth - 200, 85, 14, RAYWHITE);
        DrawText("R: Reset Mission", kSimWidth - 200, 105, 14, RAYWHITE);

        // Center Overlay: Game Over
        if (isLanded) {
            DrawRectangle(0, 0, kSimWidth, kSimHeight, Fade(BLACK, 0.4f));
            DrawText("MISSION SUCCESSFUL!", kSimWidth/2 - 180, kSimHeight/2 - 60, 30, GREEN);
            DrawText(TextFormat("SCORE: %d", score), kSimWidth/2 - 50, kSimHeight/2 - 10, 24, RAYWHITE);
            DrawText("Press 'R' for Next Mission", kSimWidth/2 - 130, kSimHeight/2 + 40, 20, LIGHTGRAY);
        } else if (isCrashed) {
            DrawRectangle(0, 0, kSimWidth, kSimHeight, Fade(RED, 0.2f));
            DrawText("MISSION FAILED: CRASHED", kSimWidth/2 - 200, kSimHeight/2 - 40, 30, RED);
            DrawText("Press 'R' to Retry", kSimWidth/2 - 80, kSimHeight/2 + 20, 20, RAYWHITE);
        } else if (fuel <= 0 && velocity.y > 0) {
             DrawText("OUT OF FUEL!", kSimWidth/2 - 60, 100, 20, RED);
        }
    }

private:
    Vector2 position;
    Vector2 velocity;
    float angle;
    float angularVelocity;
    float mass;
    
    float fuel;
    float maxFuel;
    int score;
    float landingPadX;
    float landingPadWidth;
    
    float gravity;
    float maxThrust;
    float maxTorque;
    float dragCoeff;
    
    PID velPID;
    PID anglePID;
    
    float targetDescentSpeed;
    float thrustLevel;
    
    bool isLanded;
    bool isCrashed;
    bool autoPilot;
};

} // namespace

std::unique_ptr<Simulation> CreateRocketSimulation() {
    return std::make_unique<RocketSimulation>();
}
