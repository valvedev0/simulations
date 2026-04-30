#include "simulation.hpp"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <cmath>
#include <memory>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kUiWidth = 300;
constexpr int kHeight = 640;
constexpr int kMaxAnts = 1000;
constexpr int kMaxTrails = 15000;
constexpr float kPi = 3.14159265f;
constexpr float kAntSpeed = 80.0f;
constexpr float kTurnNoise = 0.4f;
constexpr float kNestRadius = 30.0f;
constexpr float kTrailLife = 10.0f;

struct Ant {
    Vector2 position;
    float angle;
    float distance;
    bool carryingFood;
};

struct Trail {
    Vector2 position;
    float age;
    bool foodTrail;
};

struct FoodSource {
    Vector2 position;
    int amount;
    float radius;
};

struct Obstacle {
    Vector2 position;
    float radius;
};

float Distance(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float RandomFloat(float min, float max) {
    return min + (max - min) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

float AngleTo(Vector2 from, Vector2 to) {
    return atan2f(to.y - from.y, to.x - from.x);
}

float WrapRadians(float angle) {
    while (angle > kPi) {
        angle -= 2.0f * kPi;
    }
    while (angle < -kPi) {
        angle += 2.0f * kPi;
    }
    return angle;
}

Vector2 Direction(float angle) {
    return Vector2{cosf(angle), sinf(angle)};
}

class AntSimulation : public Simulation {
public:
    AntSimulation() {
        reset();
    }

    const char* name() const override { return "Ant Colony Defender (Game)"; }

    void reset() override {
        nest_ = { (kSimWidth - kUiWidth) / 2.0f, kHeight / 2.0f };
        
        ants_.clear();
        int initialAnts = 50 + (level_ * 20);
        if (initialAnts > kMaxAnts) initialAnts = kMaxAnts;
        
        for (int i = 0; i < initialAnts; ++i) {
            ants_.push_back({
                nest_,
                RandomFloat(-kPi, kPi),
                0.0f,
                false
            });
        }
        
        trails_.clear();
        obstacles_.clear();
        foodSources_.clear();

        // Generate initial food sources based on level
        for(int i=0; i < level_ + 2; ++i) {
            SpawnFood();
        }

        // Generate random natural obstacles based on level
        for(int i=0; i < level_ * 2; ++i) {
            obstacles_.push_back({
                { RandomFloat(50, kSimWidth - kUiWidth - 50), RandomFloat(50, kHeight - 50) },
                RandomFloat(20, 50)
            });
        }

        foodCollected_ = 0;
        targetFood_ = level_ * 100 + 50;
        colonyHealth_ = 100.0f;
        gameMode_ = 0; // 0 = Place Food, 1 = Draw Obstacle, 2 = Spawn Swarm
        gameOver_ = false;
        levelComplete_ = false;
    }

    void update(float deltaTime) override {
        if (IsKeyPressed(KEY_R)) {
            level_ = 1;
            score_ = 50; // Starting score
            reset();
            return;
        }
        
        if (gameOver_) return;

        if (levelComplete_) {
            if (IsKeyPressed(KEY_ENTER)) {
                level_++;
                reset();
            }
            return;
        }

        // --- INTERACTIVITY ---
        if (IsKeyPressed(KEY_ONE)) gameMode_ = 0;
        if (IsKeyPressed(KEY_TWO)) gameMode_ = 1;
        if (IsKeyPressed(KEY_THREE)) gameMode_ = 2;

        Vector2 mouse = GetMousePosition();
        bool inSimArea = (mouse.x < kSimWidth - kUiWidth);

        // Player Actions
        if (inSimArea && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (gameMode_ == 0 && score_ >= 10) { 
                // Place Food
                foodSources_.push_back({mouse, 50, 15.0f});
                score_ -= 10;
            } else if (gameMode_ == 1 && score_ >= 5) { 
                // Draw Player Obstacle
                obstacles_.push_back({mouse, 15.0f});
                score_ -= 5;
            } else if (gameMode_ == 2 && score_ >= 20) { 
                // Swarm Attack (Spawn ants)
                for(int i=0; i<10; i++) {
                    if (ants_.size() < kMaxAnts) {
                        ants_.push_back({nest_, AngleTo(nest_, mouse) + RandomFloat(-0.5f, 0.5f), 0.0f, false});
                    }
                }
                score_ -= 20;
            }
        }

        // Colony Drain (Harder each level)
        colonyHealth_ -= deltaTime * (0.5f + (level_ * 0.2f));
        if (colonyHealth_ <= 0) gameOver_ = true;

        UpdateAnts(deltaTime);
        UpdateTrails(deltaTime);

        // Win Condition
        if (foodCollected_ >= targetFood_) {
            levelComplete_ = true;
            score_ += 100 * level_;
        }
    }

    void draw() const override {
        // Dirt background
        DrawRectangle(0, 0, kSimWidth - kUiWidth, kHeight, Color{40, 35, 30, 255}); 

        DrawTrails();
        DrawObstacles();
        DrawSites();
        DrawAnts();
        DrawUI();

        // Game Overlays
        if (gameOver_) {
            DrawRectangle(0, 0, kSimWidth, kHeight, Fade(RED, 0.6f));
            DrawText("COLONY STARVED!", kSimWidth/2 - 250, kHeight/2 - 50, 50, RAYWHITE);
            DrawText(TextFormat("Final Score: %d | Level Reached: %d", score_, level_), kSimWidth/2 - 200, kHeight/2 + 20, 24, LIGHTGRAY);
            DrawText("Press 'R' to Restart", kSimWidth/2 - 140, kHeight/2 + 60, 24, RAYWHITE);
        } else if (levelComplete_) {
            DrawRectangle(0, 0, kSimWidth, kHeight, Fade(GREEN, 0.4f));
            DrawText(TextFormat("LEVEL %d COMPLETE!", level_), kSimWidth/2 - 220, kHeight/2 - 50, 50, RAYWHITE);
            DrawText(TextFormat("Score +%d", 100 * level_), kSimWidth/2 - 70, kHeight/2 + 20, 24, GOLD);
            DrawText("Press 'ENTER' to start next level", kSimWidth/2 - 200, kHeight/2 + 60, 24, RAYWHITE);
        }
    }

private:
    void SpawnFood() {
        Vector2 pos;
        // Don't spawn too close to nest
        do {
            pos = { RandomFloat(50, kSimWidth - kUiWidth - 50), RandomFloat(50, kHeight - 50) };
        } while (Distance(pos, nest_) < 150.0f);
        foodSources_.push_back({pos, GetRandomValue(50, 150), RandomFloat(15, 30)});
    }

    void UpdateAnts(float dt) {
        for (auto& ant : ants_) {
            Vector2 target = nest_;
            float desired = ant.angle;

            if (ant.carryingFood) {
                desired = AngleTo(ant.position, nest_);
            } else {
                // Look for closest food
                int closestFoodIdx = -1;
                float closestDist = 99999.0f;
                
                for (size_t i=0; i < foodSources_.size(); ++i) {
                    float d = Distance(ant.position, foodSources_[i].position);
                    if (d < foodSources_[i].radius + 60.0f && d < closestDist) {
                        closestDist = d;
                        closestFoodIdx = i;
                    }
                }

                if (closestFoodIdx != -1) {
                    desired = AngleTo(ant.position, foodSources_[closestFoodIdx].position);
                } else {
                    // Follow trail
                    float bestScore = -1.0f;
                    for (const auto& trail : trails_) {
                        if (!trail.foodTrail) continue; // Only follow food trails
                        
                        float d = Distance(ant.position, trail.position);
                        if (d > 5.0f && d < 40.0f) {
                            float angleToTrail = AngleTo(ant.position, trail.position);
                            float angleDiff = fabsf(WrapRadians(angleToTrail - ant.angle));
                            // Only look roughly forward
                            if (angleDiff < kPi / 2.0f) {
                                float s = trail.age / d;
                                if (s > bestScore) {
                                    bestScore = s;
                                    desired = angleToTrail;
                                }
                            }
                        }
                    }
                }
            }

            // Obstacle Avoidance (Raycast-like)
            for (const auto& obs : obstacles_) {
                float d = Distance(ant.position, obs.position);
                if (d < obs.radius + 15.0f) {
                    float avoidAngle = AngleTo(obs.position, ant.position);
                    desired = avoidAngle; // Run away from obstacle center
                }
            }

            // Steering dynamics
            float steering = WrapRadians(desired - ant.angle);
            ant.angle += steering * 4.0f * dt;
            ant.angle += RandomFloat(-kTurnNoise, kTurnNoise) * dt * 10.0f; // Random wander
            ant.angle = WrapRadians(ant.angle);

            Vector2 direction = Direction(ant.angle);
            Vector2 oldPosition = ant.position;
            ant.position.x += direction.x * kAntSpeed * dt;
            ant.position.y += direction.y * kAntSpeed * dt;
            ant.distance += Distance(oldPosition, ant.position);

            BounceAnt(ant);

            // Logic Interactions
            if (ant.carryingFood) {
                // Drop food at nest
                if (Distance(ant.position, nest_) < kNestRadius) {
                    ant.carryingFood = false;
                    ant.angle += kPi; // Turn around
                    foodCollected_++;
                    score_ += 2;
                    colonyHealth_ = std::min(100.0f, colonyHealth_ + 2.0f); // Heal colony
                }
            } else {
                // Pick up food
                for (auto it = foodSources_.begin(); it != foodSources_.end();) {
                    if (Distance(ant.position, it->position) < it->radius) {
                        ant.carryingFood = true;
                        ant.angle += kPi;
                        it->amount--;
                        it->radius -= 0.1f; // Shrink food source
                        
                        if (it->amount <= 0) {
                            it = foodSources_.erase(it);
                            SpawnFood(); // Spawn new food elsewhere
                        } else {
                            ++it;
                        }
                        break; // Only pick up from one at a time
                    } else {
                        ++it;
                    }
                }
            }

            MaybeAddTrail(ant);
        }
    }

    void BounceAnt(Ant& ant) {
        bool bounced = false;
        const float rightEdge = kSimWidth - kUiWidth;
        
        if (ant.position.x < 5.0f) {
            ant.position.x = 5.0f;
            bounced = true;
        } else if (ant.position.x > rightEdge - 5.0f) {
            ant.position.x = rightEdge - 5.0f;
            bounced = true;
        }

        if (ant.position.y < 5.0f) {
            ant.position.y = 5.0f;
            bounced = true;
        } else if (ant.position.y > kHeight - 5.0f) {
            ant.position.y = kHeight - 5.0f;
            bounced = true;
        }

        if (bounced) {
            ant.angle = WrapRadians(ant.angle + kPi + RandomFloat(-0.5f, 0.5f));
        }
    }

    void MaybeAddTrail(const Ant& ant) {
        if (GetRandomValue(0, 100) < 15) {
            trails_.push_back({ant.position, kTrailLife, ant.carryingFood});
            if (trails_.size() > kMaxTrails) {
                trails_.erase(trails_.begin());
            }
        }
    }

    void UpdateTrails(float dt) {
        for (auto it = trails_.begin(); it != trails_.end();) {
            it->age -= dt;
            if (it->age <= 0.0f) {
                it = trails_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void DrawTrails() const {
        for (const auto& trail : trails_) {
            float normalizedLife = trail.age / kTrailLife;
            unsigned char alpha = static_cast<unsigned char>(normalizedLife * 180);
            // Food trails are green/cyan, wander trails are faint purple
            Color color = trail.foodTrail ? Color{100, 255, 150, alpha} : Color{180, 150, 200, alpha};
            DrawPixelV(trail.position, color);
        }
    }

    void DrawObstacles() const {
        for (const auto& obs : obstacles_) {
            DrawCircleV(obs.position, obs.radius, Color{60, 60, 65, 255}); // Dark rock color
            DrawCircleLines((int)obs.position.x, (int)obs.position.y, obs.radius, BLACK);
            // Inner detail
            DrawCircleV({obs.position.x - 2, obs.position.y - 2}, obs.radius * 0.7f, Color{70, 70, 75, 255}); 
        }
    }

    void DrawSites() const {
        // Nest Hole
        DrawCircleV(nest_, kNestRadius, Color{100, 50, 20, 255}); // Brown mound
        DrawCircleV(nest_, kNestRadius * 0.5f, BLACK); // Deep hole

        // Food Sources
        for (const auto& food : foodSources_) {
            DrawCircleV(food.position, food.radius, Color{50, 200, 50, 255}); // Green leaf/food
            DrawCircleLines((int)food.position.x, (int)food.position.y, food.radius, DARKGREEN);
        }
    }

    void DrawAnts() const {
        for (const auto& ant : ants_) {
            Vector2 direction = Direction(ant.angle);
            Vector2 nose = {ant.position.x + direction.x * 5.0f, ant.position.y + direction.y * 5.0f};
            
            // Ant Body
            DrawLineEx(ant.position, nose, 2.0f, BLACK);
            DrawPixelV(ant.position, BLACK);
            
            // Carrying food visual
            if (ant.carryingFood) {
                DrawCircleV({nose.x + direction.x * 2.0f, nose.y + direction.y * 2.0f}, 2.5f, GREEN);
            }
        }
    }

    void DrawUI() const {
        const int panelX = kSimWidth - kUiWidth;
        DrawRectangle(panelX, 0, kUiWidth, kHeight, Color{25, 28, 35, 255});
        DrawLine(panelX, 0, panelX, kHeight, Color{60, 65, 75, 255});

        int y = 20;
        DrawText("COLONY DEFENDER", panelX + 20, y, 22, RAYWHITE); y += 40;
        
        DrawText(TextFormat("LEVEL %d", level_), panelX + 20, y, 24, GOLD); y += 30;
        DrawText(TextFormat("SCORE: %d", score_), panelX + 20, y, 20, RAYWHITE); y += 40;

        // Health Bar
        DrawText("COLONY HEALTH", panelX + 20, y, 14, LIGHTGRAY); y += 20;
        DrawRectangle(panelX + 20, y, 240, 15, DARKGRAY);
        DrawRectangle(panelX + 20, y, (int)(240 * (colonyHealth_/100.0f)), 15, colonyHealth_ > 30 ? GREEN : RED); y += 35;

        // Food Target Bar
        DrawText("FOOD TARGET (LEVEL UP)", panelX + 20, y, 14, LIGHTGRAY); y += 20;
        float progress = std::min(1.0f, (float)foodCollected_ / targetFood_);
        DrawRectangle(panelX + 20, y, 240, 15, DARKGRAY);
        DrawRectangle(panelX + 20, y, (int)(240 * progress), 15, SKYBLUE);
        DrawText(TextFormat("%d / %d", foodCollected_, targetFood_), panelX + 20, y + 20, 14, RAYWHITE); y += 60;

        // Abilities
        DrawText("ABILITIES (Costs Score)", panelX + 20, y, 16, GOLD); y += 30;
        
        Color c0 = gameMode_ == 0 ? YELLOW : GRAY;
        DrawText("[1] Drop Food (Cost 10)", panelX + 20, y, 16, c0); y += 25;
        
        Color c1 = gameMode_ == 1 ? YELLOW : GRAY;
        DrawText("[2] Drop Rock (Cost 5)", panelX + 20, y, 16, c1); y += 25;
        
        Color c2 = gameMode_ == 2 ? YELLOW : GRAY;
        DrawText("[3] Spawn 10 Ants (Cost 20)", panelX + 20, y, 16, c2); y += 50;

        // Instructions
        DrawText("Controls:", panelX + 20, y, 14, LIGHTGRAY); y += 20;
        DrawText("1, 2, 3: Select Ability", panelX + 20, y, 14, GRAY); y += 20;
        DrawText("Left Click: Use Ability in Dirt", panelX + 20, y, 14, GRAY); y += 20;
        DrawText("R: Restart Game", panelX + 20, y, 14, GRAY); y += 30;
        
        DrawText(TextFormat("Active Ants: %d", (int)ants_.size()), panelX + 20, kHeight - 30, 14, GRAY);
    }

    Vector2 nest_;
    std::vector<Ant> ants_;
    std::vector<Trail> trails_;
    std::vector<FoodSource> foodSources_;
    std::vector<Obstacle> obstacles_;

    int level_ = 1;
    int score_ = 50;
    int foodCollected_ = 0;
    int targetFood_;
    float colonyHealth_;
    
    int gameMode_ = 0;
    bool gameOver_ = false;
    bool levelComplete_ = false;
};

}  // namespace

std::unique_ptr<Simulation> CreateAntSimulation() {
    return std::make_unique<AntSimulation>();
}
