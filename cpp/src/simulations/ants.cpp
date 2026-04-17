#include "simulation.hpp"

#include "raylib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <numeric>
#include <vector>

namespace {

constexpr int kSimWidth = 900;
constexpr int kUiWidth = 320;
constexpr int kHeight = 640;
constexpr int kAntCount = 180;
constexpr int kTrailCount = 480;
constexpr float kPi = 3.14159265359f;
constexpr float kAntSpeed = 74.0f;
constexpr float kTurnNoise = 1.45f;
constexpr float kNestRadius = 28.0f;
constexpr float kFoodRadius = 24.0f;
constexpr float kTrailLife = 5.0f;
constexpr int kFoodBatch = 50;

struct Ant {
    Vector2 position{};
    float angle = 0.0f;
    float distance = 0.0f;
    bool carryingFood = false;
};

struct Trail {
    Vector2 position{};
    float age = 0.0f;
    bool foodTrail = false;
};

struct AntRecord {
    bool valid = false;
    float timeTaken = 0.0f;
    int frames = 0;
    int foodCollected = 0;
    float totalAntDistance = 0.0f;
    float averageAntDistance = 0.0f;
    float averageUpdateMs = 0.0f;
    float averageFrameMs = 0.0f;
};

float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Average(const std::vector<float>& values) {
    if (values.empty()) {
        return 0.0f;
    }
    return std::accumulate(values.begin(), values.end(), 0.0f) / static_cast<float>(values.size());
}

float RandomFloat(float minValue, float maxValue) {
    return minValue + static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f * (maxValue - minValue);
}

float AngleTo(Vector2 from, Vector2 to) {
    return std::atan2(to.y - from.y, to.x - from.x);
}

float WrapRadians(float angle) {
    while (angle < -kPi) {
        angle += 2.0f * kPi;
    }
    while (angle > kPi) {
        angle -= 2.0f * kPi;
    }
    return angle;
}

Vector2 Direction(float angle) {
    return Vector2{std::cos(angle), std::sin(angle)};
}

class AntSimulation final : public Simulation {
public:
    AntSimulation() {
        LoadFonts();
        reset();
    }

    ~AntSimulation() override {
        if (uiFontLoaded_) {
            UnloadFont(uiFont_);
        }
        if (titleFontLoaded_) {
            UnloadFont(titleFont_);
        }
    }

    const char* name() const override {
        return "Ant Foraging Simulation";
    }

    void reset() override {
        nest_ = Vector2{kSimWidth * 0.20f, kHeight * 0.58f};
        food_ = Vector2{kSimWidth * 0.76f, kHeight * 0.36f};
        elapsed_ = 0.0f;
        frameCount_ = 0;
        foodCollected_ = 0;
        targetFood_ = kFoodBatch;
        firstFoodTime_ = -1.0f;
        experimentComplete_ = false;
        eventLogged_ = false;
        record_ = AntRecord{};
        frameMs_.clear();
        updateMs_.clear();
        trails_.clear();
        ants_.clear();
        ants_.reserve(kAntCount);

        for (int i = 0; i < kAntCount; ++i) {
            ants_.push_back(Ant{
                nest_,
                RandomFloat(0.0f, 2.0f * kPi),
                0.0f,
                false,
            });
        }
    }

    void update(float deltaTime) override {
        const auto updateStart = std::chrono::steady_clock::now();

        if (experimentComplete_) {
            HandleContinueButton();
            return;
        }

        ++frameCount_;
        elapsed_ += deltaTime;
        frameMs_.push_back(deltaTime * 1000.0f);
        TrimSamples(frameMs_);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const Vector2 mouse = GetMousePosition();
            if (mouse.x < kSimWidth) {
                food_ = mouse;
            }
        }

        UpdateAnts(deltaTime);
        UpdateTrails(deltaTime);

        if (foodCollected_ >= targetFood_) {
            experimentComplete_ = true;
            BuildRecord();
            LogRecord();
            WriteRecordCsv();
        }

        const auto updateStop = std::chrono::steady_clock::now();
        updateMs_.push_back(std::chrono::duration<float, std::milli>(updateStop - updateStart).count());
        TrimSamples(updateMs_);
    }

    void draw() const override {
        DrawWorld();
        DrawTrails();
        DrawSites();
        DrawAnts();
        DrawDiagnostics();
    }

private:
    void LoadFonts() {
        uiFont_ = LoadFontEx("C:/Windows/Fonts/consola.ttf", 18, nullptr, 0);
        titleFont_ = LoadFontEx("C:/Windows/Fonts/consolab.ttf", 24, nullptr, 0);
        uiFontLoaded_ = uiFont_.texture.id != 0;
        titleFontLoaded_ = titleFont_.texture.id != 0;

        if (uiFontLoaded_) {
            SetTextureFilter(uiFont_.texture, TEXTURE_FILTER_BILINEAR);
        }
        if (titleFontLoaded_) {
            SetTextureFilter(titleFont_.texture, TEXTURE_FILTER_BILINEAR);
        }
    }

    void TrimSamples(std::vector<float>& samples) const {
        constexpr int maxSamples = 120;
        if (samples.size() > maxSamples) {
            samples.erase(samples.begin(), samples.begin() + static_cast<int>(samples.size()) - maxSamples);
        }
    }

    void UpdateAnts(float deltaTime) {
        for (Ant& ant : ants_) {
            const Vector2 oldPosition = ant.position;
            const Vector2 target = ant.carryingFood ? nest_ : food_;
            const float desired = AngleTo(ant.position, target);
            const float steering = WrapRadians(desired - ant.angle);
            const float targetWeight = ant.carryingFood ? 3.6f : 1.15f;
            ant.angle += steering * targetWeight * deltaTime + RandomFloat(-kTurnNoise, kTurnNoise) * deltaTime;

            const Vector2 direction = Direction(ant.angle);
            ant.position.x += direction.x * kAntSpeed * deltaTime;
            ant.position.y += direction.y * kAntSpeed * deltaTime;

            BounceAnt(ant);
            ant.distance += Distance(oldPosition, ant.position);

            if (!ant.carryingFood && Distance(ant.position, food_) <= kFoodRadius) {
                ant.carryingFood = true;
                if (firstFoodTime_ < 0.0f) {
                    firstFoodTime_ = elapsed_;
                }
            }

            if (ant.carryingFood && Distance(ant.position, nest_) <= kNestRadius) {
                ant.carryingFood = false;
                ++foodCollected_;
            }

            MaybeAddTrail(ant);
        }
    }

    void BounceAnt(Ant& ant) {
        if (ant.position.x < 0.0f) {
            ant.position.x = 0.0f;
            ant.angle = kPi - ant.angle;
        } else if (ant.position.x > kSimWidth) {
            ant.position.x = static_cast<float>(kSimWidth);
            ant.angle = kPi - ant.angle;
        }

        if (ant.position.y < 0.0f) {
            ant.position.y = 0.0f;
            ant.angle = -ant.angle;
        } else if (ant.position.y > kHeight) {
            ant.position.y = static_cast<float>(kHeight);
            ant.angle = -ant.angle;
        }
    }

    void MaybeAddTrail(const Ant& ant) {
        if (GetRandomValue(0, 100) > 10) {
            return;
        }

        if (trails_.size() >= kTrailCount) {
            trails_.erase(trails_.begin());
        }

        trails_.push_back(Trail{ant.position, 0.0f, ant.carryingFood});
    }

    void UpdateTrails(float deltaTime) {
        for (Trail& trail : trails_) {
            trail.age += deltaTime;
        }

        trails_.erase(
            std::remove_if(trails_.begin(), trails_.end(), [](const Trail& trail) {
                return trail.age > kTrailLife;
            }),
            trails_.end());
    }

    void BuildRecord() {
        record_.valid = true;
        record_.timeTaken = elapsed_;
        record_.frames = frameCount_;
        record_.foodCollected = foodCollected_;
        record_.totalAntDistance = 0.0f;

        for (const Ant& ant : ants_) {
            record_.totalAntDistance += ant.distance;
        }

        record_.averageAntDistance = record_.totalAntDistance / static_cast<float>(ants_.size());
        record_.averageFrameMs = Average(frameMs_);
        record_.averageUpdateMs = Average(updateMs_);
    }

    void LogRecord() {
        if (eventLogged_ || !record_.valid) {
            return;
        }

        TraceLog(
            LOG_INFO,
            "ANTS_TARGET_REACHED time=%.3fs frames=%d food=%d total_ant_distance=%.2fpx average_ant_distance=%.2fpx first_food_time=%.3fs avg_frame_ms=%.3f avg_update_ms=%.3f",
            record_.timeTaken,
            record_.frames,
            record_.foodCollected,
            record_.totalAntDistance,
            record_.averageAntDistance,
            firstFoodTime_,
            record_.averageFrameMs,
            record_.averageUpdateMs);
        eventLogged_ = true;
    }

    void WriteRecordCsv() const {
        if (!record_.valid) {
            return;
        }

        std::ofstream file("ants_target_event.csv", std::ios::app);
        if (!file) {
            TraceLog(LOG_WARNING, "Could not write ants_target_event.csv");
            return;
        }

        if (file.tellp() == 0) {
            file << "event,time_taken_s,frames,food_collected,ant_count,nest_x,nest_y,food_x,food_y,total_ant_distance_px,average_ant_distance_px,first_food_time_s,avg_frame_ms,avg_update_ms\n";
        }

        file << "ANTS_TARGET_REACHED,"
             << record_.timeTaken << ','
             << record_.frames << ','
             << record_.foodCollected << ','
             << ants_.size() << ','
             << nest_.x << ','
             << nest_.y << ','
             << food_.x << ','
             << food_.y << ','
             << record_.totalAntDistance << ','
             << record_.averageAntDistance << ','
             << firstFoodTime_ << ','
             << record_.averageFrameMs << ','
             << record_.averageUpdateMs
             << '\n';
    }

    Rectangle ContinueButtonBounds() const {
        return Rectangle{static_cast<float>(kSimWidth + 14), static_cast<float>(kHeight - 54), 138.0f, 36.0f};
    }

    void HandleContinueButton() {
        const Rectangle bounds = ContinueButtonBounds();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), bounds)) {
            targetFood_ += kFoodBatch;
            experimentComplete_ = false;
            eventLogged_ = false;
        }
    }

    void DrawWorld() const {
        DrawRectangle(0, 0, kSimWidth, kHeight, Color{18, 24, 22, 255});
    }

    void DrawTrails() const {
        for (const Trail& trail : trails_) {
            const float normalizedLife = std::clamp(1.0f - trail.age / kTrailLife, 0.0f, 1.0f);
            const unsigned char alpha = static_cast<unsigned char>(normalizedLife * 150.0f);
            const Color color = trail.foodTrail ? Color{230, 186, 73, alpha} : Color{96, 171, 126, alpha};
            DrawCircleV(trail.position, 2.0f, color);
        }
    }

    void DrawSites() const {
        DrawCircleV(nest_, kNestRadius, Color{84, 150, 94, 255});
        DrawCircleV(food_, kFoodRadius, Color{226, 190, 75, 255});
        DrawCircleLines(static_cast<int>(nest_.x), static_cast<int>(nest_.y), kNestRadius + 5.0f, Color{147, 211, 153, 255});
        DrawCircleLines(static_cast<int>(food_.x), static_cast<int>(food_.y), kFoodRadius + 5.0f, Color{240, 217, 117, 255});
    }

    void DrawAnts() const {
        for (const Ant& ant : ants_) {
            const Vector2 direction = Direction(ant.angle);
            const Vector2 nose{ant.position.x + direction.x * 6.0f, ant.position.y + direction.y * 6.0f};
            const Color color = ant.carryingFood ? Color{248, 212, 86, 255} : Color{190, 207, 222, 255};
            DrawCircleV(ant.position, 2.6f, color);
            DrawLineV(ant.position, nose, color);
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
        const float progress = static_cast<float>(foodCollected_) / static_cast<float>(targetFood_);

        DrawRectangle(panelX, 0, kUiWidth, kHeight, Color{12, 12, 16, 255});
        DrawLineEx(Vector2{static_cast<float>(panelX), 0.0f}, Vector2{static_cast<float>(panelX), static_cast<float>(kHeight)}, 2.0f, Color{90, 90, 100, 255});
        DrawTitle(x, y, "ANT DIAGNOSTICS");
        y += 34;

        DrawMono(x, y, TextFormat("Status: %s", experimentComplete_ ? "TARGET_REACHED" : "RUNNING"), 15.0f, experimentComplete_ ? Color{84, 231, 166, 255} : Color{235, 235, 240, 255}); y += 22;
        DrawMono(x, y, TextFormat("Time: %7.2f s", elapsed_)); y += 18;
        DrawMono(x, y, TextFormat("FPS: %6.2f", fps)); y += 18;
        DrawMono(x, y, TextFormat("Frame avg: %7.3f ms", Average(frameMs_))); y += 18;
        DrawMono(x, y, TextFormat("Update avg:%7.3f ms", Average(updateMs_))); y += 26;

        DrawMono(x, y, TextFormat("Ants: %d", static_cast<int>(ants_.size()))); y += 18;
        DrawMono(x, y, TextFormat("Food: %d / %d", foodCollected_, targetFood_)); y += 18;
        DrawMono(x, y, TextFormat("Progress: %6.2f %%", progress * 100.0f)); y += 18;
        DrawMono(x, y, TextFormat("First food: %7.2f s", firstFoodTime_ < 0.0f ? 0.0f : firstFoodTime_)); y += 18;
        DrawMono(x, y, TextFormat("Trail points: %d", static_cast<int>(trails_.size()))); y += 26;

        const float totalDistance = TotalAntDistance();
        DrawMono(x, y, TextFormat("Total distance: %8.1f", totalDistance)); y += 18;
        DrawMono(x, y, TextFormat("Avg ant dist:  %8.1f", totalDistance / static_cast<float>(ants_.size()))); y += 26;

        if (record_.valid) {
            DrawMono(x, y, "EVENT RECORD", 15.0f, Color{84, 231, 166, 255}); y += 20;
            DrawMono(x, y, TextFormat("time_taken: %7.3f s", record_.timeTaken)); y += 18;
            DrawMono(x, y, TextFormat("food:       %7d", record_.foodCollected)); y += 18;
            DrawMono(x, y, TextFormat("avg_dist:   %7.1f", record_.averageAntDistance)); y += 18;
            DrawMono(x, y, "saved: ants_target_event.csv"); y += 18;
            DrawContinueButton();
        } else {
            DrawMono(x, y, "Controls:", 15.0f, Color{235, 235, 240, 255}); y += 18;
            DrawMono(x, y, "LMB: move food target"); y += 18;
            DrawMono(x, y, "R: reset"); y += 18;
            DrawMono(x, y, "ESC: quit/menu");
        }
    }

    void DrawContinueButton() const {
        if (!experimentComplete_) {
            return;
        }

        const Rectangle bounds = ContinueButtonBounds();
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        DrawRectangleRec(bounds, hovered ? Color{112, 205, 154, 255} : Color{84, 181, 130, 255});
        DrawRectangleLinesEx(bounds, 1.0f, Color{177, 239, 198, 255});
        DrawMono(static_cast<int>(bounds.x + 26.0f), static_cast<int>(bounds.y + 9.0f), "Continue", 15.0f, Color{8, 16, 12, 255});
    }

    float TotalAntDistance() const {
        float total = 0.0f;
        for (const Ant& ant : ants_) {
            total += ant.distance;
        }
        return total;
    }

    Vector2 nest_{};
    Vector2 food_{};
    std::vector<Ant> ants_;
    std::vector<Trail> trails_;
    std::vector<float> frameMs_;
    std::vector<float> updateMs_;
    Font uiFont_{};
    Font titleFont_{};
    bool uiFontLoaded_ = false;
    bool titleFontLoaded_ = false;
    bool experimentComplete_ = false;
    bool eventLogged_ = false;
    int foodCollected_ = 0;
    int frameCount_ = 0;
    int targetFood_ = kFoodBatch;
    float elapsed_ = 0.0f;
    float firstFoodTime_ = -1.0f;
    AntRecord record_;
};

}  // namespace

std::unique_ptr<Simulation> CreateAntSimulation() {
    return std::make_unique<AntSimulation>();
}
