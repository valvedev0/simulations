#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <vector>
#include <string>
#include <cmath>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kSimHeight = 640;

enum TransmissionMode {
    AUTO,
    MANUAL
};

struct Car {
    Vector3 position;
    Vector3 velocity;
    float heading; 
    
    // Engine & Transmission
    float speed; // Actual velocity magnitude
    float engineRPM;
    int currentGear; // 0 = Neutral, 1-5 = Forward, -1 = Reverse
    TransmissionMode transMode;
    
    Color color;
    int currentWaypoint;
    bool isAI;
};

struct Obstacle {
    Vector3 position;
    Vector3 size;
    Color color;
};

// Gear ratios for a typical 5-speed
const float GEAR_RATIOS[] = { 0.0f, 3.5f, 2.0f, 1.4f, 1.0f, 0.7f }; 
const float REVERSE_RATIO = 3.5f;
const float FINAL_DRIVE = 3.4f;
const float WHEEL_RADIUS = 0.4f;
const float IDLE_RPM = 1000.0f;
const float REDLINE_RPM = 7000.0f;

class RacingSimulation : public Simulation {
public:
    RacingSimulation() {
        camera.position = { 0.0f, 5.0f, -10.0f };
        camera.target = { 0.0f, 0.0f, 0.0f };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        reset();
    }

    ~RacingSimulation() override {
    }

    const char* name() const override { return "3D Racing (Sim Physics)"; }

    void reset() override {
        playerCar = {
            { 0.0f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
            0.0f, IDLE_RPM, 1, AUTO, BLUE, 0, false
        };

        aiCar = {
            { 5.0f, 0.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f,
            0.0f, IDLE_RPM, 1, AUTO, RED, 0, true
        };

        waypoints.clear();
        int numWaypoints = 12;
        float radius = 250.0f; // Larger track for higher speeds
        for (int i = 0; i < numWaypoints; ++i) {
            float angle = (float)i / numWaypoints * 2.0f * PI;
            waypoints.push_back({ cosf(angle) * radius, 0.5f, sinf(angle) * radius });
        }

        obstacles.clear();
        for (int i = 0; i < 80; ++i) {
            float ox = (float)GetRandomValue(-400, 400);
            float oz = (float)GetRandomValue(-400, 400);
            if (sqrt(ox*ox + oz*oz) < 30.0f) continue;

            obstacles.push_back({
                { ox, 2.0f, oz },
                { (float)GetRandomValue(3, 8), (float)GetRandomValue(3, 12), (float)GetRandomValue(3, 8) },
                ColorFromHSV((float)GetRandomValue(0, 360), 0.6f, 0.7f)
            });
        }

        raceStarted = false;
        raceTimer = 0.0f;
    }

    void update(float deltaTime) override {
        if (IsKeyPressed(KEY_R)) reset();

        if (!raceStarted) {
            raceTimer += deltaTime;
            if (raceTimer > 3.0f) {
                raceStarted = true;
                raceTimer = 0.0f;
            }
            UpdateCarPhysics(playerCar, 0.0f, 0.0f, deltaTime); // Idle RPM update
            UpdateCameraBehindPlayer();
            return;
        }

        raceTimer += deltaTime;

        // --- Player Input ---
        float throttle = 0.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttle = 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttle = -1.0f; // Brake/Reverse
        
        float steerInput = 0.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) steerInput = 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) steerInput = -1.0f;

        // Transmission Controls
        if (IsKeyPressed(KEY_T)) {
            playerCar.transMode = (playerCar.transMode == AUTO) ? MANUAL : AUTO;
        }
        if (playerCar.transMode == MANUAL) {
            if (IsKeyPressed(KEY_E) && playerCar.currentGear < 5) playerCar.currentGear++;
            if (IsKeyPressed(KEY_Q) && playerCar.currentGear > -1) playerCar.currentGear--;
        }

        UpdateCarPhysics(playerCar, throttle, steerInput, deltaTime);

        // --- AI Logic ---
        float aiThrottle = 0.0f;
        float aiSteer = 0.0f;
        
        Vector3 targetWp = waypoints[aiCar.currentWaypoint];
        Vector3 toTarget = Vector3Subtract(targetWp, aiCar.position);
        float distToTarget = Vector3Length(toTarget);

        if (distToTarget > 0.1f) {
            float desiredHeading = atan2f(-toTarget.z, toTarget.x);
            float diff = desiredHeading - aiCar.heading;
            while (diff < -PI) diff += 2.0f * PI;
            while (diff > PI) diff -= 2.0f * PI;

            aiSteer = diff * 2.5f; 
            if (aiSteer > 1.0f) aiSteer = 1.0f;
            if (aiSteer < -1.0f) aiSteer = -1.0f;

            if (abs(diff) > PI/4.0f && aiCar.speed > 15.0f) {
                aiThrottle = -1.0f; 
            } else {
                aiThrottle = 1.0f;
            }
        }
        UpdateCarPhysics(aiCar, aiThrottle, aiSteer, deltaTime);

        CheckWaypoint(playerCar);
        CheckWaypoint(aiCar);
        HandleCollisions(playerCar);
        HandleCollisions(aiCar);
        UpdateCameraBehindPlayer();
    }

    void draw() const override {
        BeginMode3D(camera);

        DrawGrid(200, 10.0f);

        for (size_t i = 0; i < waypoints.size(); ++i) {
            Color c = Fade(YELLOW, 0.3f);
            if (i == (size_t)playerCar.currentWaypoint) c = Fade(GREEN, 0.6f);
            DrawCylinder(waypoints[i], 8.0f, 8.0f, 30.0f, 16, c);
            DrawCylinderWires(waypoints[i], 8.0f, 8.0f, 30.0f, 16, ORANGE);
        }

        for (const auto& obs : obstacles) {
            DrawCubeV(obs.position, obs.size, obs.color);
            DrawCubeWiresV(obs.position, obs.size, DARKGRAY);
        }

        DrawCar(playerCar);
        DrawCar(aiCar);

        EndMode3D();

        // --- UI Overlay ---
        DrawRectangle(10, 10, 260, 200, Fade(BLACK, 0.85f));
        DrawText("3D RACING SIMULATOR", 20, 20, 18, SKYBLUE);
        
        if (!raceStarted) {
            int countdown = 3 - (int)raceTimer;
            DrawText(TextFormat("STARTING IN %d...", countdown), kSimWidth/2 - 150, kSimHeight/2, 40, RED);
        } else {
            // Speed Calculation (m/s to km/h)
            float kmh = playerCar.speed * 3.6f;
            
            DrawText(TextFormat("Time: %.2f s", raceTimer), 20, 50, 18, LIGHTGRAY);
            DrawText(TextFormat("SPEED: %03.0f km/h", abs(kmh)), 20, 80, 24, GREEN);
            DrawText(TextFormat("RPM:   %04.0f", playerCar.engineRPM), 20, 110, 18, (playerCar.engineRPM > 6500) ? RED : RAYWHITE);
            
            std::string gearStr = (playerCar.currentGear == -1) ? "R" : (playerCar.currentGear == 0) ? "N" : std::to_string(playerCar.currentGear);
            std::string transStr = (playerCar.transMode == AUTO) ? "AUTO" : "MANUAL";
            DrawText(TextFormat("GEAR:  %s  [%s]", gearStr.c_str(), transStr.c_str()), 20, 140, 18, ORANGE);
            
            DrawText(TextFormat("Checkpoint: %d / %d", playerCar.currentWaypoint, (int)waypoints.size()), 20, 170, 18, YELLOW);
        }

        // Draw RPM Bar
        int barWidth = 220;
        float rpmRatio = (playerCar.engineRPM - IDLE_RPM) / (REDLINE_RPM - IDLE_RPM);
        if (rpmRatio < 0) rpmRatio = 0;
        if (rpmRatio > 1) rpmRatio = 1;
        DrawRectangle(20, 195, barWidth, 6, DARKGRAY);
        DrawRectangle(20, 195, (int)(barWidth * rpmRatio), 6, (rpmRatio > 0.9f) ? RED : LIME);

        // Controls hint
        DrawRectangle(10, kSimHeight - 70, 380, 60, Fade(BLACK, 0.7f));
        DrawText("W/S: Throttle/Brake  A/D: Steer  R: Reset", 20, kSimHeight - 60, 16, GRAY);
        DrawText("T: Toggle Auto/Manual  Q/E: Shift Down/Up", 20, kSimHeight - 40, 16, GRAY);
    }

private:
    Camera3D camera;
    Car playerCar;
    Car aiCar;
    std::vector<Vector3> waypoints;
    std::vector<Obstacle> obstacles;
    
    bool raceStarted;
    float raceTimer;

    // Advanced Engine Torque Curve Approximation
    float GetEngineTorque(float rpm) {
        if (rpm < IDLE_RPM) return 0.0f;
        if (rpm > REDLINE_RPM) return 0.0f;
        // Peak torque around 4500 RPM
        float peakRPM = 4500.0f;
        float peakTorque = 400.0f; // Nm
        float dropOff = abs(rpm - peakRPM) / 3000.0f;
        return peakTorque * (1.0f - dropOff * dropOff * 0.5f);
    }

    void UpdateCarPhysics(Car& car, float throttle, float steerInput, float dt) {
        float mass = 1200.0f; // kg
        float aeroDrag = 0.45f;
        float rollingResistance = 12.0f;
        float brakingForce = 8000.0f;

        // Auto Transmission Logic
        if (car.transMode == AUTO) {
            if (throttle > 0 && car.currentGear < 1) car.currentGear = 1;
            if (throttle < 0 && car.speed < 1.0f) car.currentGear = -1; // Switch to reverse if stopped
            
            if (car.currentGear > 0 && car.currentGear < 5 && car.engineRPM > 6200.0f) {
                car.currentGear++; // Shift Up
            }
            if (car.currentGear > 1 && car.engineRPM < 3000.0f) {
                car.currentGear--; // Shift Down
            }
        }

        // Calculate Engine RPM from wheel speed if in gear
        float gearRatio = 0.0f;
        if (car.currentGear > 0) gearRatio = GEAR_RATIOS[car.currentGear];
        if (car.currentGear == -1) gearRatio = REVERSE_RATIO;

        if (car.currentGear != 0) {
            car.engineRPM = (abs(car.speed) / (2.0f * PI * WHEEL_RADIUS)) * gearRatio * FINAL_DRIVE * 60.0f;
            if (car.engineRPM < IDLE_RPM) car.engineRPM = IDLE_RPM;
        } else {
            // Rev engine freely in neutral
            car.engineRPM += throttle * 5000.0f * dt;
            if (car.engineRPM > REDLINE_RPM) car.engineRPM = REDLINE_RPM;
            if (throttle <= 0.0f) car.engineRPM -= 2000.0f * dt;
            if (car.engineRPM < IDLE_RPM) car.engineRPM = IDLE_RPM;
        }

        float tractionForce = 0.0f;

        // Apply Throttle/Brake
        if (throttle > 0.0f) {
            if (car.currentGear > 0) {
                float torque = GetEngineTorque(car.engineRPM) * throttle;
                tractionForce = (torque * gearRatio * FINAL_DRIVE) / WHEEL_RADIUS;
            } else if (car.currentGear == -1) {
                // Braking while in reverse
                tractionForce = -brakingForce;
            }
        } else if (throttle < 0.0f) {
            if (car.currentGear > 0) {
                tractionForce = -brakingForce; // Braking
            } else if (car.currentGear == -1) {
                // Throttle in reverse
                float torque = GetEngineTorque(car.engineRPM) * abs(throttle);
                tractionForce = -(torque * gearRatio * FINAL_DRIVE) / WHEEL_RADIUS;
            }
        }

        // Drag & Rolling Resistance
        float dragForce = 0.5f * aeroDrag * (car.speed * car.speed);
        float rrForce = rollingResistance * car.speed;
        
        // Ensure drag forces oppose motion
        if (car.speed < 0) { dragForce = -dragForce; rrForce = -rrForce; }
        
        float totalForce = tractionForce - dragForce - rrForce;
        float acceleration = totalForce / mass;

        // Update Speed
        car.speed += acceleration * dt;

        // Hard stop if speed is very low and no throttle
        if (abs(car.speed) < 0.5f && throttle == 0.0f) {
            car.speed = 0.0f;
        }

        // Steering (requires movement)
        if (abs(car.speed) > 1.0f) {
            // Turning is less effective at very high speeds
            float turnSpeed = 2.0f / (1.0f + abs(car.speed) * 0.02f); 
            float dir = (car.speed > 0) ? 1.0f : -1.0f;
            car.heading += steerInput * turnSpeed * dir * dt;
        }

        // Apply Velocity
        car.velocity.x = cosf(car.heading) * car.speed;
        car.velocity.z = -sinf(car.heading) * car.speed; 

        car.position.x += car.velocity.x * dt;
        car.position.z += car.velocity.z * dt;
    }

    void HandleCollisions(Car& car) {
        float carRadius = 1.5f;
        for (const auto& obs : obstacles) {
            float dx = car.position.x - obs.position.x;
            float dz = car.position.z - obs.position.z;
            float dist = sqrt(dx*dx + dz*dz);
            float obsRadius = (obs.size.x + obs.size.z) / 4.0f; 

            if (dist < carRadius + obsRadius) {
                float pushForce = (carRadius + obsRadius - dist);
                car.position.x += (dx/dist) * pushForce;
                car.position.z += (dz/dist) * pushForce;
                car.speed *= 0.6f; // Severe loss of speed on impact
                car.engineRPM *= 0.8f;
            }
        }
    }

    void CheckWaypoint(Car& car) {
        if (car.currentWaypoint >= (int)waypoints.size()) return;

        Vector3 wp = waypoints[car.currentWaypoint];
        float dx = car.position.x - wp.x;
        float dz = car.position.z - wp.z;
        float dist = sqrt(dx*dx + dz*dz);

        if (dist < 12.0f) { 
            car.currentWaypoint++;
            if (car.currentWaypoint >= (int)waypoints.size()) {
                car.currentWaypoint = 0;
            }
        }
    }

    void UpdateCameraBehindPlayer() {
        // Dynamic camera that pulls back as speed increases
        float speedRatio = abs(playerCar.speed) / 50.0f;
        float camDistance = 10.0f + (speedRatio * 6.0f);
        float camHeight = 4.0f + (speedRatio * 2.0f);
        
        Vector3 desiredCamPos = {
            playerCar.position.x - cosf(playerCar.heading) * camDistance,
            playerCar.position.y + camHeight,
            playerCar.position.z + sinf(playerCar.heading) * camDistance
        };

        camera.position.x += (desiredCamPos.x - camera.position.x) * 0.15f;
        camera.position.y += (desiredCamPos.y - camera.position.y) * 0.1f;
        camera.position.z += (desiredCamPos.z - camera.position.z) * 0.15f;

        Vector3 lookAhead = {
            playerCar.position.x + cosf(playerCar.heading) * 10.0f,
            playerCar.position.y,
            playerCar.position.z - sinf(playerCar.heading) * 10.0f
        };

        camera.target = lookAhead;
    }

    void DrawCar(const Car& car) const {
        rlPushMatrix();
        rlTranslatef(car.position.x, car.position.y, car.position.z);
        rlRotatef(car.heading * RAD2DEG, 0.0f, 1.0f, 0.0f);

        // Tilt body slightly based on acceleration/braking
        float tilt = (car.speed > 5.0f) ? -2.0f : 0.0f;
        rlRotatef(tilt, 0.0f, 0.0f, 1.0f);

        // Main Body 
        DrawCube({ 0.0f, 0.5f, 0.0f }, 4.0f, 1.0f, 2.0f, car.color);
        DrawCubeWires({ 0.0f, 0.5f, 0.0f }, 4.0f, 1.0f, 2.0f, BLACK);

        // Cabin
        DrawCube({ -0.5f, 1.25f, 0.0f }, 2.0f, 0.5f, 1.8f, DARKGRAY);
        DrawCubeWires({ -0.5f, 1.25f, 0.0f }, 2.0f, 0.5f, 1.8f, BLACK);

        // Wheels
        DrawCylinderEx({ 1.2f, 0.3f, 1.1f }, { 1.2f, 0.3f, 1.3f }, WHEEL_RADIUS, WHEEL_RADIUS, 8, BLACK);  
        DrawCylinderEx({ 1.2f, 0.3f, -1.1f }, { 1.2f, 0.3f, -1.3f }, WHEEL_RADIUS, WHEEL_RADIUS, 8, BLACK); 
        DrawCylinderEx({ -1.2f, 0.3f, 1.1f }, { -1.2f, 0.3f, 1.3f }, WHEEL_RADIUS, WHEEL_RADIUS, 8, BLACK); 
        DrawCylinderEx({ -1.2f, 0.3f, -1.1f }, { -1.2f, 0.3f, -1.3f }, WHEEL_RADIUS, WHEEL_RADIUS, 8, BLACK);

        // Headlights / Taillights
        DrawCube({ 1.9f, 0.6f, 0.6f }, 0.2f, 0.3f, 0.5f, YELLOW);
        DrawCube({ 1.9f, 0.6f, -0.6f }, 0.2f, 0.3f, 0.5f, YELLOW);
        DrawCube({ -1.9f, 0.6f, 0.6f }, 0.2f, 0.3f, 0.5f, RED);
        DrawCube({ -1.9f, 0.6f, -0.6f }, 0.2f, 0.3f, 0.5f, RED);

        rlPopMatrix();
    }
};

} // namespace

std::unique_ptr<Simulation> CreateRacingSimulation() {
    return std::make_unique<RacingSimulation>();
}
