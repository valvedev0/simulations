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
constexpr int kStarCount = 120;
constexpr float kMaxSpeed = 380.0f;
constexpr float kAcceleration = 240.0f;
constexpr float kDrag = 0.14f;
constexpr float kTurnSpeedDeg = 120.0f;
constexpr float kPi = 3.14159265359f;
constexpr float kTargetRadius = 20.0f;

struct Star {
    Vector2 position{};
    float brightness = 0.0f;
    float radius = 1.0f;
};

struct EventCounters {
    int total = 0;
    int quit = 0;
    int keyDown = 0;
    int keyUp = 0;
    int mouseDown = 0;
    int mouseMotion = 0;
};

struct MissionRecord {
    bool valid = false;
    float timeTaken = 0.0f;
    int frames = 0;
    Vector2 startPosition{};
    Vector2 finalPosition{};
    Vector2 targetPosition{};
    float finalTargetDistance = 0.0f;
    float straightLineDistance = 0.0f;
    float pathDistance = 0.0f;
    float pathEfficiency = 0.0f;
    float maxSpeed = 0.0f;
    float averageSpeed = 0.0f;
    float finalHeading = 0.0f;
    float leftTurnTime = 0.0f;
    float rightTurnTime = 0.0f;
    float straightTime = 0.0f;
    int directionChanges = 0;
    float averageFrameMs = 0.0f;
    float averageUpdateMs = 0.0f;
    float averageRenderMs = 0.0f;
};

float DegreesToRadians(float degrees) {
    return degrees * kPi / 180.0f;
}

float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Length(Vector2 value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

Vector2 Add(Vector2 a, Vector2 b) {
    return Vector2{a.x + b.x, a.y + b.y};
}

Vector2 Subtract(Vector2 a, Vector2 b) {
    return Vector2{a.x - b.x, a.y - b.y};
}

Vector2 Scale(Vector2 value, float scale) {
    return Vector2{value.x * scale, value.y * scale};
}

float HeadingToTarget(Vector2 from, Vector2 to) {
    const Vector2 delta{to.x - from.x, to.y - from.y};
    return std::atan2(delta.y, delta.x) * 180.0f / kPi;
}

float NormalizeAngle(float angle) {
    while (angle < 0.0f) {
        angle += 360.0f;
    }
    while (angle >= 360.0f) {
        angle -= 360.0f;
    }
    return angle;
}

float ShortestAngleDelta(float from, float to) {
    float delta = NormalizeAngle(to) - NormalizeAngle(from);
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }
    return delta;
}

float Average(const std::vector<float>& values) {
    if (values.empty()) {
        return 0.0f;
    }
    return std::accumulate(values.begin(), values.end(), 0.0f) / static_cast<float>(values.size());
}

class FlightSimulation final : public Simulation {
public:
    FlightSimulation() {
        LoadFonts();
        reset();
    }

    ~FlightSimulation() override {
        if (uiFontLoaded_) {
            UnloadFont(uiFont_);
        }
        if (titleFontLoaded_) {
            UnloadFont(titleFont_);
        }
    }

    const char* name() const override {
        return "Interactive Flight Simulation";
    }

    void reset() override {
        position_ = Vector2{kSimWidth * 0.5f, kHeight * 0.6f};
        startPosition_ = position_;
        velocity_ = Vector2{0.0f, 0.0f};
        headingDeg_ = -90.0f;
        throttle_ = 0.25f;
        autopilot_ = true;
        targetPosition_ = Vector2{kSimWidth * 0.7f, kHeight * 0.3f};
        elapsed_ = 0.0f;
        frameCount_ = 0;
        distanceTraveled_ = 0.0f;
        maxSpeedSeen_ = 0.0f;
        leftTurnTime_ = 0.0f;
        rightTurnTime_ = 0.0f;
        straightTime_ = 0.0f;
        directionChanges_ = 0;
        lastDirection_ = 0;
        missionComplete_ = false;
        eventLogged_ = false;
        targetArmed_ = true;
        record_ = MissionRecord{};
        events_ = EventCounters{};
        lastMousePosition_ = GetMousePosition();
        frameMs_.clear();
        updateMs_.clear();
        renderMs_.clear();
        GenerateStars();
    }

    void update(float deltaTime) override {
        const auto updateStart = std::chrono::steady_clock::now();

        if (missionComplete_) {
            CountEvents();
            HandleContinueButton();
            return;
        }

        ++frameCount_;
        elapsed_ += deltaTime;
        frameMs_.push_back(deltaTime * 1000.0f);
        TrimSamples(frameMs_);

        CountEvents();
        ApplyControls(deltaTime);
        Integrate(deltaTime);
        UpdateTargetArming();
        CheckMissionComplete();

        const auto updateStop = std::chrono::steady_clock::now();
        const float updateMs = std::chrono::duration<float, std::milli>(updateStop - updateStart).count();
        updateMs_.push_back(updateMs);
        TrimSamples(updateMs_);
    }

    void draw() const override {
        const auto renderStart = std::chrono::steady_clock::now();

        DrawWorld();
        DrawWaypoint();
        DrawAircraft();
        DrawDiagnostics();

        const auto renderStop = std::chrono::steady_clock::now();
        const float renderMs = std::chrono::duration<float, std::milli>(renderStop - renderStart).count();
        renderMs_.push_back(renderMs);
        TrimSamples(renderMs_);
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

    void GenerateStars() {
        stars_.clear();
        stars_.reserve(kStarCount);

        for (int i = 0; i < kStarCount; ++i) {
            stars_.push_back(Star{
                Vector2{
                    static_cast<float>(GetRandomValue(0, kSimWidth - 1)),
                    static_cast<float>(GetRandomValue(0, kHeight - 1)),
                },
                static_cast<float>(GetRandomValue(1, 2)),
                1.0f,
            });
        }
    }

    void TrimSamples(std::vector<float>& samples) const {
        constexpr int maxSamples = 120;
        if (samples.size() > maxSamples) {
            samples.erase(samples.begin(), samples.begin() + static_cast<int>(samples.size()) - maxSamples);
        }
    }

    void CountEvents() {
        const Vector2 mouse = GetMousePosition();
        if (std::abs(mouse.x - lastMousePosition_.x) > 0.01f || std::abs(mouse.y - lastMousePosition_.y) > 0.01f) {
            ++events_.mouseMotion;
            ++events_.total;
            lastMousePosition_ = mouse;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            ++events_.mouseDown;
            ++events_.total;
        }

        const int watchedKeys[] = {
            KEY_W, KEY_S, KEY_UP, KEY_DOWN, KEY_A, KEY_D, KEY_LEFT, KEY_RIGHT,
            KEY_P, KEY_R, KEY_SPACE,
        };

        for (int key : watchedKeys) {
            if (IsKeyPressed(key)) {
                ++events_.keyDown;
                ++events_.total;
            }
            if (IsKeyReleased(key)) {
                ++events_.keyUp;
                ++events_.total;
            }
        }
    }

    void ApplyControls(float deltaTime) {
        float turnInput = 0.0f;
        float throttleDelta = 0.0f;
        bool braking = false;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const Vector2 mouse = GetMousePosition();
            if (mouse.x < static_cast<float>(kSimWidth)) {
                targetPosition_ = mouse;
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_P)) {
            autopilot_ = !autopilot_;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            velocity_ = Vector2{0.0f, 0.0f};
        }

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            turnInput -= 1.0f;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            turnInput += 1.0f;
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
            throttleDelta += 0.60f;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
            throttleDelta -= 0.60f;
            braking = true;
        }

        if (autopilot_) {
            const Vector2 toTarget = Subtract(targetPosition_, position_);
            if (Length(toTarget) > 0.0f) {
                const float desiredHeading = HeadingToTarget(position_, targetPosition_);
                const float delta = ShortestAngleDelta(headingDeg_, desiredHeading);
                if (std::abs(delta) > 1.2f) {
                    turnInput += delta > 0.0f ? 1.0f : -1.0f;
                }
                throttle_ = std::clamp(throttle_, 0.35f, 0.75f);
            }
        }

        if (std::abs(turnInput) > 0.01f) {
            headingDeg_ += turnInput * kTurnSpeedDeg * deltaTime;
        }

        RecordDirection(turnInput, deltaTime);
        headingDeg_ = NormalizeAngle(headingDeg_);
        throttle_ = std::clamp(throttle_ + throttleDelta * deltaTime, 0.0f, 1.0f);
        brakingThisFrame_ = braking;
    }

    void RecordDirection(float turnInput, float deltaTime) {
        int direction = 0;
        if (turnInput < -0.01f) {
            direction = -1;
            leftTurnTime_ += deltaTime;
        } else if (turnInput > 0.01f) {
            direction = 1;
            rightTurnTime_ += deltaTime;
        } else {
            straightTime_ += deltaTime;
        }

        if (direction != 0 && lastDirection_ != 0 && direction != lastDirection_) {
            ++directionChanges_;
        }

        if (direction != 0) {
            lastDirection_ = direction;
        }
    }

    void Integrate(float deltaTime) {
        const Vector2 oldPosition = position_;
        const Vector2 forward = ForwardVector();

        velocity_ = Add(velocity_, Scale(forward, kAcceleration * throttle_ * deltaTime));

        if (brakingThisFrame_) {
            velocity_ = Scale(velocity_, 0.96f);
        }

        const float dragFactor = std::max(0.0f, 1.0f - kDrag * deltaTime);
        velocity_ = Scale(velocity_, dragFactor);

        float speed = Length(velocity_);
        if (speed > kMaxSpeed) {
            velocity_ = Scale(velocity_, kMaxSpeed / speed);
            speed = kMaxSpeed;
        }

        position_ = Add(position_, Scale(velocity_, deltaTime));

        BounceFromBoundaries();

        distanceTraveled_ += Distance(oldPosition, position_);
        maxSpeedSeen_ = std::max(maxSpeedSeen_, speed);
    }

    void CheckMissionComplete() {
        if (missionComplete_ || !targetArmed_ || Distance(position_, targetPosition_) >= kTargetRadius) {
            return;
        }

        velocity_ = Vector2{0.0f, 0.0f};
        throttle_ = 0.0f;
        autopilot_ = false;
        missionComplete_ = true;
        BuildMissionRecord();
        LogMissionEvent();
        WriteMissionRecordCsv();
    }

    void UpdateTargetArming() {
        if (!targetArmed_ && Distance(position_, targetPosition_) > kTargetRadius * 1.5f) {
            targetArmed_ = true;
            eventLogged_ = false;
        }
    }

    Rectangle ContinueButtonBounds() const {
        return Rectangle{static_cast<float>(kSimWidth + 14), static_cast<float>(kHeight - 54), 138.0f, 36.0f};
    }

    void HandleContinueButton() {
        const Rectangle bounds = ContinueButtonBounds();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), bounds)) {
            missionComplete_ = false;
            targetArmed_ = false;
            autopilot_ = false;
            throttle_ = 0.0f;
            velocity_ = Vector2{0.0f, 0.0f};
        }
    }

    void BuildMissionRecord() {
        record_.valid = true;
        record_.timeTaken = elapsed_;
        record_.frames = frameCount_;
        record_.startPosition = startPosition_;
        record_.finalPosition = position_;
        record_.targetPosition = targetPosition_;
        record_.finalTargetDistance = Distance(position_, targetPosition_);
        record_.straightLineDistance = Distance(startPosition_, targetPosition_);
        record_.pathDistance = distanceTraveled_;
        record_.pathEfficiency = distanceTraveled_ > 0.0f ? record_.straightLineDistance / distanceTraveled_ : 0.0f;
        record_.maxSpeed = maxSpeedSeen_;
        record_.averageSpeed = elapsed_ > 0.0f ? distanceTraveled_ / elapsed_ : 0.0f;
        record_.finalHeading = NormalizeAngle(headingDeg_);
        record_.leftTurnTime = leftTurnTime_;
        record_.rightTurnTime = rightTurnTime_;
        record_.straightTime = straightTime_;
        record_.directionChanges = directionChanges_;
        record_.averageFrameMs = Average(frameMs_);
        record_.averageUpdateMs = Average(updateMs_);
        record_.averageRenderMs = Average(renderMs_);
    }

    void LogMissionEvent() {
        if (eventLogged_ || !record_.valid) {
            return;
        }

        TraceLog(
            LOG_INFO,
            "TARGET_REACHED time=%.3fs frames=%d path=%.2fpx direct=%.2fpx efficiency=%.3f max_speed=%.2fpx/s avg_speed=%.2fpx/s final_distance=%.2fpx heading=%.2fdeg left_turn=%.3fs right_turn=%.3fs straight=%.3fs direction_changes=%d avg_frame_ms=%.3f avg_update_ms=%.3f avg_render_ms=%.3f",
            record_.timeTaken,
            record_.frames,
            record_.pathDistance,
            record_.straightLineDistance,
            record_.pathEfficiency,
            record_.maxSpeed,
            record_.averageSpeed,
            record_.finalTargetDistance,
            record_.finalHeading,
            record_.leftTurnTime,
            record_.rightTurnTime,
            record_.straightTime,
            record_.directionChanges,
            record_.averageFrameMs,
            record_.averageUpdateMs,
            record_.averageRenderMs);
        eventLogged_ = true;
    }

    void WriteMissionRecordCsv() const {
        if (!record_.valid) {
            return;
        }

        std::ofstream file("flight_target_event.csv", std::ios::app);
        if (!file) {
            TraceLog(LOG_WARNING, "Could not write flight_target_event.csv");
            return;
        }

        if (file.tellp() == 0) {
            file << "event,time_taken_s,frames,start_x,start_y,target_x,target_y,final_x,final_y,final_target_distance_px,path_distance_px,direct_distance_px,path_efficiency,max_speed_px_s,average_speed_px_s,final_heading_deg,left_turn_s,right_turn_s,straight_s,direction_changes,avg_frame_ms,avg_update_ms,avg_render_ms\n";
        }

        file << "TARGET_REACHED,"
             << record_.timeTaken << ','
             << record_.frames << ','
             << record_.startPosition.x << ','
             << record_.startPosition.y << ','
             << record_.targetPosition.x << ','
             << record_.targetPosition.y << ','
             << record_.finalPosition.x << ','
             << record_.finalPosition.y << ','
             << record_.finalTargetDistance << ','
             << record_.pathDistance << ','
             << record_.straightLineDistance << ','
             << record_.pathEfficiency << ','
             << record_.maxSpeed << ','
             << record_.averageSpeed << ','
             << record_.finalHeading << ','
             << record_.leftTurnTime << ','
             << record_.rightTurnTime << ','
             << record_.straightTime << ','
             << record_.directionChanges << ','
             << record_.averageFrameMs << ','
             << record_.averageUpdateMs << ','
             << record_.averageRenderMs
             << '\n';
    }

    void BounceFromBoundaries() {
        const float width = static_cast<float>(kSimWidth);
        const float height = static_cast<float>(kHeight);
        if (position_.x < 0.0f) {
            position_.x = 0.0f;
            velocity_.x *= -0.5f;
        } else if (position_.x > width) {
            position_.x = width;
            velocity_.x *= -0.5f;
        }

        if (position_.y < 0.0f) {
            position_.y = 0.0f;
            velocity_.y *= -0.5f;
        } else if (position_.y > height) {
            position_.y = height;
            velocity_.y *= -0.5f;
        }
    }

    Vector2 ForwardVector() const {
        const float radians = DegreesToRadians(headingDeg_);
        return Vector2{std::cos(radians), std::sin(radians)};
    }

    void DrawWorld() const {
        DrawRectangle(0, 0, kSimWidth, kHeight, Color{16, 20, 28, 255});

        for (const Star& star : stars_) {
            DrawCircleV(star.position, star.brightness, Color{180, 190, 210, 255});
        }
    }

    void DrawWaypoint() const {
        DrawCircleLines(static_cast<int>(targetPosition_.x), static_cast<int>(targetPosition_.y), 8.0f, Color{240, 210, 80, 255});
        DrawCircleV(targetPosition_, 2.0f, Color{240, 210, 80, 255});
    }

    void DrawAircraft() const {
        const Vector2 forward = ForwardVector();
        const Vector2 right{-forward.y, forward.x};
        const Vector2 nose = Add(position_, Scale(forward, 18.0f));
        const Vector2 p2 = Add(Subtract(position_, Scale(forward, 10.0f)), Scale(right, 8.0f));
        const Vector2 p3 = Subtract(Subtract(position_, Scale(forward, 10.0f)), Scale(right, 8.0f));

        DrawLineEx(position_, nose, 2.0f, RAYWHITE);
        DrawTriangle(
            nose,
            p2,
            p3,
            Color{120, 220, 255, 255});

        if (missionComplete_) {
            DrawCircleLines(static_cast<int>(position_.x), static_cast<int>(position_.y), 24.0f, Color{84, 231, 166, 255});
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

        DrawRectangle(panelX, 0, kUiWidth, kHeight, Color{12, 12, 16, 255});
        DrawLineEx(Vector2{static_cast<float>(panelX), 0.0f}, Vector2{static_cast<float>(panelX), static_cast<float>(kHeight)}, 2.0f, Color{90, 90, 100, 255});

        DrawTitle(x, y, "FLIGHT DIAGNOSTICS");
        y += 32;

        const float fps = Average(frameMs_) > 0.0f ? 1000.0f / Average(frameMs_) : 0.0f;
        const float speed = Length(velocity_);
        const float heading = NormalizeAngle(headingDeg_);
        const float targetDistance = Distance(position_, targetPosition_);

        DrawMono(x, y, TextFormat("Status: %s", missionComplete_ ? "TARGET_REACHED" : "RUNNING"), 15.0f, missionComplete_ ? Color{84, 231, 166, 255} : Color{235, 235, 240, 255}); y += 22;
        DrawMono(x, y, TextFormat("Autopilot: %s", autopilot_ ? "ON" : "OFF")); y += 18;
        DrawMono(x, y, TextFormat("Time: %7.2f s", elapsed_)); y += 18;
        DrawMono(x, y, TextFormat("FPS: %6.2f", fps)); y += 18;
        DrawMono(x, y, TextFormat("Frame avg: %7.3f ms", Average(frameMs_))); y += 18;
        DrawMono(x, y, TextFormat("Update avg:%7.3f ms", Average(updateMs_))); y += 18;
        DrawMono(x, y, TextFormat("Render avg:%7.3f ms", Average(renderMs_))); y += 26;

        DrawMono(x, y, TextFormat("Pos: (%7.1f, %7.1f)", position_.x, position_.y)); y += 18;
        DrawMono(x, y, TextFormat("Target: (%5.1f, %5.1f)", targetPosition_.x, targetPosition_.y)); y += 18;
        DrawMono(x, y, TextFormat("Target dist: %7.2f", targetDistance)); y += 18;
        DrawMono(x, y, TextFormat("Speed: %7.2f px/s", speed)); y += 18;
        DrawMono(x, y, TextFormat("Heading: %7.2f deg", heading)); y += 18;
        DrawMono(x, y, TextFormat("Path: %8.1f px", distanceTraveled_)); y += 18;
        DrawMono(x, y, TextFormat("Max speed: %7.2f", maxSpeedSeen_)); y += 26;

        DrawMono(x, y, "Directions:"); y += 18;
        DrawMono(x, y, TextFormat("Left:     %7.2f s", leftTurnTime_)); y += 18;
        DrawMono(x, y, TextFormat("Right:    %7.2f s", rightTurnTime_)); y += 18;
        DrawMono(x, y, TextFormat("Straight: %7.2f s", straightTime_)); y += 18;
        DrawMono(x, y, TextFormat("Changes:  %7d", directionChanges_)); y += 30;

        if (record_.valid) {
            DrawMono(x, y, "EVENT RECORD", 15.0f, Color{84, 231, 166, 255}); y += 20;
            DrawMono(x, y, TextFormat("time_taken: %7.3f s", record_.timeTaken)); y += 18;
            DrawMono(x, y, TextFormat("path_dist:  %7.2f px", record_.pathDistance)); y += 18;
            DrawMono(x, y, TextFormat("direct_dist:%7.2f px", record_.straightLineDistance)); y += 18;
            DrawMono(x, y, TextFormat("efficiency: %7.3f", record_.pathEfficiency)); y += 18;
            DrawMono(x, y, TextFormat("avg_speed:  %7.2f", record_.averageSpeed)); y += 18;
            DrawMono(x, y, TextFormat("final_dist: %7.2f px", record_.finalTargetDistance)); y += 18;
            DrawMono(x, y, "saved: flight_target_event.csv"); y += 18;
            DrawContinueButton();
        } else {
            DrawMono(x, y, "Controls:", 15.0f, Color{235, 235, 240, 255}); y += 18;
            DrawMono(x, y, "LMB: set target"); y += 18;
            DrawMono(x, y, "P/RMB: autopilot"); y += 18;
            DrawMono(x, y, "R: reset"); y += 18;
            DrawMono(x, y, "ESC: quit/menu");
        }
    }

    void DrawContinueButton() const {
        if (!missionComplete_) {
            return;
        }

        const Rectangle bounds = ContinueButtonBounds();
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        DrawRectangleRec(bounds, hovered ? Color{112, 205, 154, 255} : Color{84, 181, 130, 255});
        DrawRectangleLinesEx(bounds, 1.0f, Color{177, 239, 198, 255});
        DrawMono(static_cast<int>(bounds.x + 26.0f), static_cast<int>(bounds.y + 9.0f), "Continue", 15.0f, Color{8, 16, 12, 255});
    }

    Vector2 position_{};
    Vector2 velocity_{};
    Vector2 startPosition_{};
    Vector2 targetPosition_{};
    Vector2 lastMousePosition_{};
    bool autopilot_ = false;
    bool brakingThisFrame_ = false;
    bool missionComplete_ = false;
    bool eventLogged_ = false;
    bool targetArmed_ = true;
    float headingDeg_ = 270.0f;
    float throttle_ = 0.25f;
    float elapsed_ = 0.0f;
    float distanceTraveled_ = 0.0f;
    float maxSpeedSeen_ = 0.0f;
    float leftTurnTime_ = 0.0f;
    float rightTurnTime_ = 0.0f;
    float straightTime_ = 0.0f;
    int directionChanges_ = 0;
    int lastDirection_ = 0;
    int frameCount_ = 0;
    EventCounters events_;
    MissionRecord record_;
    Font uiFont_{};
    Font titleFont_{};
    bool uiFontLoaded_ = false;
    bool titleFontLoaded_ = false;
    std::vector<Star> stars_;
    std::vector<float> frameMs_;
    std::vector<float> updateMs_;
    mutable std::vector<float> renderMs_;
};

}  // namespace

std::unique_ptr<Simulation> CreateFlightSimulation() {
    return std::make_unique<FlightSimulation>();
}
