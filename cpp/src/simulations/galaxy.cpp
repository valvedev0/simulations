#include "simulation.hpp"
#include "raylib.h"
#include "rlgl.h" 
#include "raymath.h"

#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kSimHeight = 640;
constexpr int kNumParticles = 50000; // Background star dust
constexpr int kNumPlanets = 8;
constexpr int kTrailLength = 50;

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float size;
};

struct Planet {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float radius;
    float mass;
    std::vector<Vector2> trail;
    int trailIndex;
};

class GalaxySimulation : public Simulation {
public:
    GalaxySimulation() {
        reset();
    }

    const char* name() const override { return "Interactive GPU Galaxy"; }

    void reset() override {
        // --- 1. Generate Star Dust (GPU Particles) ---
        particles.clear();
        particles.reserve(kNumParticles);
        
        Vector2 center = { kSimWidth / 2.0f, kSimHeight / 2.0f };

        for (int i = 0; i < kNumParticles; ++i) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            
            // Create spiral arms effect using normally distributed randomness
            float radius = (float)GetRandomValue(20, 800);
            float spiralOffset = angle + (radius * 0.005f); // Twist the arms
            
            // Add some scatter so it's not a perfect line
            float scatter = (float)GetRandomValue(-40, 40);
            
            float velocityMagnitude = sqrtf(150000.0f / (radius + 10.0f)); 
            
            particles.push_back({
                { center.x + cosf(spiralOffset) * radius + scatter, center.y + sinf(spiralOffset) * radius + scatter },
                { -sinf(angle) * velocityMagnitude, cosf(angle) * velocityMagnitude },
                GetRandomStarColor(),
                (float)GetRandomValue(1, 15) / 10.0f // Size between 0.1 and 1.5
            });
        }
        
        // --- 2. Generate Planets ---
        planets.clear();
        for (int i = 0; i < kNumPlanets; ++i) {
            float radius = (float)GetRandomValue(100, 400);
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float vMag = sqrtf(150000.0f / radius);
            
            Planet p;
            p.position = { center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
            p.velocity = { -sinf(angle) * vMag, cosf(angle) * vMag };
            p.radius = (float)GetRandomValue(4, 12);
            p.mass = p.radius * 2.0f; // Roughly proportional to size
            p.color = ColorFromHSV((float)GetRandomValue(0, 360), 0.8f, 0.9f);
            p.trail.resize(kTrailLength, p.position);
            p.trailIndex = 0;
            planets.push_back(p);
        }

        blackHoleMass = 150000.0f;
        blackHolePos = center;
        repelForce = 0.0f;
    }

    void update(float deltaTime) override {
        if (IsKeyPressed(KEY_R)) reset();

        float dt = deltaTime;
        if (dt > 0.05f) dt = 0.05f; // Cap delta time

        // --- INTERACTIVITY ---
        // Left Click: Drag Black Hole
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            blackHolePos = GetMousePosition();
        }
        
        // Right Click: Supernova Repulsion
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            repelForce = 500000.0f;
        } else {
            repelForce = 0.0f;
        }

        // Scroll Wheel: Change Black Hole Mass
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            blackHoleMass += wheel * 20000.0f;
            if (blackHoleMass < 10000.0f) blackHoleMass = 10000.0f;
            if (blackHoleMass > 500000.0f) blackHoleMass = 500000.0f;
        }

        // --- UPDATE PLANETS ---
        for (auto& p : planets) {
            Vector2 dir = Vector2Subtract(blackHolePos, p.position);
            float distSq = (dir.x * dir.x) + (dir.y * dir.y);
            if (distSq < 400.0f) distSq = 400.0f; // Prevent singularity glitches
            
            float force = blackHoleMass / distSq;
            if (repelForce > 0) force -= repelForce / distSq;

            Vector2 accel = Vector2Scale(Vector2Normalize(dir), force);
            
            p.velocity = Vector2Add(p.velocity, Vector2Scale(accel, dt));
            p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));

            // Update trail
            p.trailIndex = (p.trailIndex + 1) % kTrailLength;
            p.trail[p.trailIndex] = p.position;
        }

        // --- UPDATE STAR DUST ---
        for (auto& p : particles) {
            Vector2 dir = Vector2Subtract(blackHolePos, p.position);
            float distSq = (dir.x * dir.x) + (dir.y * dir.y);
            if (distSq < 100.0f) distSq = 100.0f;
            
            float force = blackHoleMass / distSq;
            if (repelForce > 0) force -= (repelForce * 2.0f) / distSq; // Dust flies away faster

            Vector2 accel = Vector2Scale(Vector2Normalize(dir), force);
            
            p.velocity = Vector2Add(p.velocity, Vector2Scale(accel, dt));
            
            // Add a tiny bit of drag so they eventually spiral in instead of orbiting perfectly forever
            p.velocity = Vector2Scale(p.velocity, 0.999f); 
            
            p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
        }
    }

    void draw() const override {
        // Deep space background
        DrawRectangle(0, 0, kSimWidth, kSimHeight, Color{5, 5, 12, 255});

        // --- HIGH PERFORMANCE GPU PARTICLE RENDERING ---
        // Enable additive blending for a glowing nebula effect
        BeginBlendMode(BLEND_ADDITIVE);
        
        rlDrawRenderBatchActive(); 
        rlBegin(RL_QUADS); 
        
        for (const auto& p : particles) {
            // Speed-based color (faster = brighter/bluer)
            float speed = Vector2Length(p.velocity);
            unsigned char alpha = (unsigned char)std::clamp((int)(speed * 1.5f), 50, 255);
            rlColor4ub(p.color.r, p.color.g, p.color.b, alpha);
            
            float s = p.size;
            rlVertex2f(p.position.x - s, p.position.y - s);
            rlVertex2f(p.position.x - s, p.position.y + s);
            rlVertex2f(p.position.x + s, p.position.y + s);
            rlVertex2f(p.position.x + s, p.position.y - s);
        }
        rlEnd(); 
        EndBlendMode(); // Return to normal blending
        // --- END GPU RENDERING ---

        // --- DRAW PLANETS AND TRAILS ---
        for (const auto& p : planets) {
            // Draw trail
            for (int i = 0; i < kTrailLength - 1; ++i) {
                int idx1 = (p.trailIndex - i + kTrailLength) % kTrailLength;
                int idx2 = (p.trailIndex - i - 1 + kTrailLength) % kTrailLength;
                
                // Fade out trail
                float alpha = 1.0f - ((float)i / kTrailLength);
                Color tColor = Fade(p.color, alpha * 0.5f);
                DrawLineEx(p.trail[idx1], p.trail[idx2], p.radius * alpha * 0.5f, tColor);
            }
            // Draw planet body
            DrawCircleV(p.position, p.radius, p.color);
            // Atmosphere glow
            DrawCircleGradient((int)p.position.x, (int)p.position.y, p.radius * 1.8f, Fade(p.color, 0.4f), Fade(p.color, 0.0f));
        }

        // --- DRAW BLACK HOLE ---
        float eventHorizon = sqrtf(blackHoleMass) / 10.0f;
        
        // Accretion disk glow
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleGradient((int)blackHolePos.x, (int)blackHolePos.y, eventHorizon * 4.0f, Color{150, 50, 255, 100}, BLANK);
        DrawCircleGradient((int)blackHolePos.x, (int)blackHolePos.y, eventHorizon * 2.0f, Color{255, 150, 50, 150}, BLANK);
        EndBlendMode();
        
        // The Void
        DrawCircleV(blackHolePos, eventHorizon, BLACK);
        DrawCircleLines((int)blackHolePos.x, (int)blackHolePos.y, eventHorizon, Color{80, 20, 150, 255});
        
        if (repelForce > 0) {
            DrawCircleLines((int)blackHolePos.x, (int)blackHolePos.y, eventHorizon * 1.5f + (float)sin(GetTime()*20)*5.0f, RED);
        }

        // --- UI ---
        DrawRectangle(10, 10, 260, 140, Fade(BLACK, 0.8f));
        DrawText("INTERACTIVE GALAXY", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Star Dust: %d", kNumParticles), 20, 50, 14, LIGHTGRAY);
        DrawText(TextFormat("Planets: %d", kNumPlanets), 20, 70, 14, LIGHTGRAY);
        DrawText(TextFormat("Mass: %.0f", blackHoleMass), 20, 90, 14, SKYBLUE);
        
        DrawText("LMB: Move Black Hole", kSimWidth - 220, 20, 16, PURPLE);
        DrawText("RMB: Supernova Repel", kSimWidth - 220, 45, 16, RED);
        DrawText("Scroll: Change Mass", kSimWidth - 220, 70, 16, SKYBLUE);
    }

private:
    Color GetRandomStarColor() const {
        int r = GetRandomValue(0, 100);
        if (r < 60) return Color{ 150, 200, 255, 255 }; // Bright Blue
        if (r < 85) return Color{ 255, 240, 200, 255 }; // Yellow-White
        if (r < 95) return Color{ 255, 150, 100, 255 }; // Red Dwarf
        return Color{ 255, 255, 255, 255 };             // Pure White
    }

    std::vector<Particle> particles;
    std::vector<Planet> planets;
    
    Vector2 blackHolePos;
    float blackHoleMass;
    float repelForce;
};

} // namespace

std::unique_ptr<Simulation> CreateGalaxySimulation() {
    return std::make_unique<GalaxySimulation>();
}
