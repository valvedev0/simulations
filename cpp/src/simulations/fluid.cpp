#include "simulation.hpp"

#include "raylib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

namespace {

constexpr int kSimWidth = 900;
constexpr int kUiWidth = 320;
constexpr int kHeight = 640;
constexpr int kParticleCount = 2500;
constexpr float kRadius = 4.0f;
constexpr float kInteractionRadius = 10.0f;
constexpr int kCellSize = 10;
constexpr int kGridCols = kSimWidth / kCellSize + 1;
constexpr int kGridRows = kHeight / kCellSize + 1;
constexpr float kGravity = 500.0f;
constexpr float kDamping = 0.5f;

struct Particle {
    Vector2 pos{};
    Vector2 vel{};
    Color color{};
};

float Average(const std::vector<float>& values) {
    if (values.empty()) {
        return 0.0f;
    }
    return std::accumulate(values.begin(), values.end(), 0.0f) / static_cast<float>(values.size());
}

class FluidSimulation final : public Simulation {
public:
    FluidSimulation() {
        LoadFonts();
        reset();
    }

    ~FluidSimulation() override {
        if (uiFontLoaded_) UnloadFont(uiFont_);
        if (titleFontLoaded_) UnloadFont(titleFont_);
    }

    const char* name() const override {
        return "Interactive Fluid Simulation";
    }

    void reset() override {
        particles_.clear();
        particles_.reserve(kParticleCount);
        for (int i = 0; i < kParticleCount; ++i) {
            Particle p;
            p.pos.x = static_cast<float>(GetRandomValue(kSimWidth / 2 - 150, kSimWidth / 2 + 150));
            p.pos.y = static_cast<float>(GetRandomValue(50, 400));
            p.vel = {0.0f, 0.0f};
            p.color = Color{
                static_cast<unsigned char>(GetRandomValue(40, 60)),
                static_cast<unsigned char>(GetRandomValue(140, 190)),
                static_cast<unsigned char>(GetRandomValue(220, 255)),
                255
            };
            particles_.push_back(p);
        }
        elapsed_ = 0.0f;
        frameCount_ = 0;
        frameMs_.clear();
        updateMs_.clear();
        renderMs_.clear();
    }

    void update(float deltaTime) override {
        const auto updateStart = std::chrono::steady_clock::now();

        if (deltaTime > 0.033f) {
            deltaTime = 0.033f;
        }

        ++frameCount_;
        elapsed_ += deltaTime;
        frameMs_.push_back(deltaTime * 1000.0f);
        TrimSamples(frameMs_);

        ApplyGravity(deltaTime);
        ApplyInteraction(deltaTime);
        ResolveCollisions();
        Integrate(deltaTime);

        const auto updateStop = std::chrono::steady_clock::now();
        updateMs_.push_back(std::chrono::duration<float, std::milli>(updateStop - updateStart).count());
        TrimSamples(updateMs_);
    }

    void draw() const override {
        const auto renderStart = std::chrono::steady_clock::now();

        DrawWorld();
        DrawParticles();
        DrawDiagnostics();

        const auto renderStop = std::chrono::steady_clock::now();
        renderMs_.push_back(std::chrono::duration<float, std::milli>(renderStop - renderStart).count());
        TrimSamples(renderMs_);
    }

private:
    void LoadFonts() {
        uiFont_ = LoadFontEx("C:/Windows/Fonts/consola.ttf", 18, nullptr, 0);
        titleFont_ = LoadFontEx("C:/Windows/Fonts/consolab.ttf", 24, nullptr, 0);
        uiFontLoaded_ = uiFont_.texture.id != 0;
        titleFontLoaded_ = titleFont_.texture.id != 0;

        if (uiFontLoaded_) SetTextureFilter(uiFont_.texture, TEXTURE_FILTER_BILINEAR);
        if (titleFontLoaded_) SetTextureFilter(titleFont_.texture, TEXTURE_FILTER_BILINEAR);
    }

    void TrimSamples(std::vector<float>& samples) const {
        constexpr int maxSamples = 120;
        if (samples.size() > maxSamples) {
            samples.erase(samples.begin(), samples.begin() + static_cast<int>(samples.size()) - maxSamples);
        }
    }

    void ApplyGravity(float deltaTime) {
        for (auto& p : particles_) {
            p.vel.y += kGravity * deltaTime;
        }
    }

    void ApplyInteraction(float deltaTime) {
        const Vector2 mouse = GetMousePosition();
        const bool left = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        const bool right = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

        if (!left && !right) return;
        if (mouse.x > static_cast<float>(kSimWidth)) return;

        const float interactionDist = 100.0f;
        for (auto& p : particles_) {
            const float dx = p.pos.x - mouse.x;
            const float dy = p.pos.y - mouse.y;
            const float distSq = dx * dx + dy * dy;

            if (distSq < interactionDist * interactionDist && distSq > 1.0f) {
                const float dist = std::sqrt(distSq);
                const Vector2 dir = {dx / dist, dy / dist};
                float force = (interactionDist - dist) * 25.0f;
                if (right) force = -force; // Attract

                p.vel.x += dir.x * force * deltaTime;
                p.vel.y += dir.y * force * deltaTime;
            }
        }
    }

    void ResolveCollisions() {
        for (int x = 0; x < kGridCols; ++x) {
            for (int y = 0; y < kGridRows; ++y) {
                grid_[x][y].clear();
            }
        }

        for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
            const int cx = std::clamp(static_cast<int>(particles_[i].pos.x / kCellSize), 0, kGridCols - 1);
            const int cy = std::clamp(static_cast<int>(particles_[i].pos.y / kCellSize), 0, kGridRows - 1);
            grid_[cx][cy].push_back(i);
        }

        const float min_dist = kInteractionRadius;
        const float min_dist_sq = min_dist * min_dist;

        for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
            const int cx = std::clamp(static_cast<int>(particles_[i].pos.x / kCellSize), 0, kGridCols - 1);
            const int cy = std::clamp(static_cast<int>(particles_[i].pos.y / kCellSize), 0, kGridRows - 1);

            for (int nx = cx - 1; nx <= cx + 1; ++nx) {
                for (int ny = cy - 1; ny <= cy + 1; ++ny) {
                    if (nx >= 0 && nx < kGridCols && ny >= 0 && ny < kGridRows) {
                        for (int j : grid_[nx][ny]) {
                            if (i >= j) continue;

                            const float dx = particles_[i].pos.x - particles_[j].pos.x;
                            const float dy = particles_[i].pos.y - particles_[j].pos.y;
                            const float distSq = dx * dx + dy * dy;

                            if (distSq > 0.0001f && distSq < min_dist_sq) {
                                const float dist = std::sqrt(distSq);
                                const float overlap = min_dist - dist;
                                const float nx_norm = dx / dist;
                                const float ny_norm = dy / dist;

                                // Push particles apart to satisfy incompressibility
                                const float push = overlap * 0.5f;
                                particles_[i].pos.x += nx_norm * push;
                                particles_[i].pos.y += ny_norm * push;
                                particles_[j].pos.x -= nx_norm * push;
                                particles_[j].pos.y -= ny_norm * push;

                                // Viscosity and velocity exchange
                                const float rvx = particles_[i].vel.x - particles_[j].vel.x;
                                const float rvy = particles_[i].vel.y - particles_[j].vel.y;
                                const float visc = 0.08f;
                                particles_[i].vel.x -= rvx * visc;
                                particles_[i].vel.y -= rvy * visc;
                                particles_[j].vel.x += rvx * visc;
                                particles_[j].vel.y += rvy * visc;
                            }
                        }
                    }
                }
            }
        }
    }

    void Integrate(float deltaTime) {
        for (auto& p : particles_) {
            p.vel.x *= 0.998f;
            p.vel.y *= 0.998f;

            p.pos.x += p.vel.x * deltaTime;
            p.pos.y += p.vel.y * deltaTime;

            if (p.pos.x < kRadius) { p.pos.x = kRadius; p.vel.x *= -kDamping; }
            if (p.pos.x > kSimWidth - kRadius) { p.pos.x = kSimWidth - kRadius; p.vel.x *= -kDamping; }
            if (p.pos.y > kHeight - kRadius) { p.pos.y = kHeight - kRadius; p.vel.y *= -kDamping; }
            if (p.pos.y < kRadius) { p.pos.y = kRadius; p.vel.y *= -kDamping; }
        }
    }

    void DrawWorld() const {
        DrawRectangle(0, 0, kSimWidth, kHeight, Color{16, 20, 28, 255});
    }

    void DrawParticles() const {
        for (const auto& p : particles_) {
            DrawCircleV(p.pos, kRadius, p.color);
        }
    }

    void DrawMono(int x, int y, const char* text, float fontSize = 15.0f, Color color = Color{210, 210, 210, 255}) const {
        if (uiFontLoaded_) {
            DrawTextEx(uiFont_, text, Vector2{static_cast<float>(x), static_cast<float>(y)}, fontSize, 1.0f, color);
        } else {
            DrawText(text, x, y, static_cast<int>(fontSize), color);
        }
    }

    void DrawTitle(int x, int y, const char* text) const {
        if (titleFontLoaded_) {
            DrawTextEx(titleFont_, text, Vector2{static_cast<float>(x), static_cast<float>(y)}, 20.0f, 2.0f, RAYWHITE);
        } else {
            DrawText(text, x, y, 18, RAYWHITE);
        }
    }

    void DrawDiagnostics() const {
        const int panelX = kSimWidth;
        const int x = panelX + 14;
        int y = 16;
        const float fps = Average(frameMs_) > 0.0f ? 1000.0f / Average(frameMs_) : 0.0f;

        DrawRectangle(panelX, 0, kUiWidth, kHeight, Color{12, 12, 16, 255});
        DrawLineEx(Vector2{static_cast<float>(panelX), 0.0f}, Vector2{static_cast<float>(panelX), static_cast<float>(kHeight)}, 2.0f, Color{90, 90, 100, 255});
        DrawTitle(x, y, "FLUID DIAGNOSTICS");
        y += 34;

        DrawMono(x, y, TextFormat("Status: RUNNING"), 15.0f, Color{84, 231, 166, 255}); y += 22;
        DrawMono(x, y, TextFormat("Time: %7.2f s", elapsed_)); y += 18;
        DrawMono(x, y, TextFormat("FPS: %6.2f", fps)); y += 18;
        DrawMono(x, y, TextFormat("Frame avg: %7.3f ms", Average(frameMs_))); y += 18;
        DrawMono(x, y, TextFormat("Update avg:%7.3f ms", Average(updateMs_))); y += 18;
        DrawMono(x, y, TextFormat("Render avg:%7.3f ms", Average(renderMs_))); y += 26;

        DrawMono(x, y, TextFormat("Particles: %d", kParticleCount)); y += 26;

        DrawMono(x, y, "Controls:", 15.0f, Color{235, 235, 240, 255}); y += 18;
        DrawMono(x, y, "LMB: Repel fluid"); y += 18;
        DrawMono(x, y, "RMB: Attract fluid"); y += 18;
        DrawMono(x, y, "R: Reset simulation"); y += 18;
        DrawMono(x, y, "ESC: Quit/Menu");
    }

    std::vector<Particle> particles_;
    std::vector<int> grid_[kGridCols][kGridRows];
    std::vector<float> frameMs_;
    std::vector<float> updateMs_;
    mutable std::vector<float> renderMs_;

    Font uiFont_{};
    Font titleFont_{};
    bool uiFontLoaded_ = false;
    bool titleFontLoaded_ = false;

    int frameCount_ = 0;
    float elapsed_ = 0.0f;
};

}  // namespace

std::unique_ptr<Simulation> CreateFluidSimulation() {
    return std::make_unique<FluidSimulation>();
}
