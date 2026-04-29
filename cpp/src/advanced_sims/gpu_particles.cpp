#include "external/glad.h"   // Must be included before raylib.h on desktop to expose modern OpenGL functions
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include <vector>
#include <iostream>

// Force high-performance GPU on Windows (Nvidia Optimus & AMD CrossFire)
#if defined(_WIN32)
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

const int screenWidth = 1600;
const int screenHeight = 900;
const int numParticles = 2000000; // 2 Million particles for RTX 4060 / GTX 1060

struct Particle {
    float x, y;
    float vx, vy;
    float r, g, b, a;
};

// Compute Shader: The GPU does 100% of the physics math here
const char* computeShaderSrc = R"(
#version 430 core
layout(local_size_x = 256) in;

struct Particle {
    float x, y;
    float vx, vy;
    float r, g, b, a;
};

// SSBO Binding
layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform vec2 attractor;
uniform float dt;
uniform float repel;
uniform float attractForce;
uniform float forceMultiplier;
uniform float time;
uniform int colorMode;

uniform vec2 shipPos;
uniform float shipRadius;

// Simple pseudo-random function for GPU
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index >= 2000000) return;

    vec2 p = vec2(particles[index].x, particles[index].y);
    vec2 v = vec2(particles[index].vx, particles[index].vy);

    // --- ENEMY WAR-SHIP COLLISION ---
    vec2 shipDir = p - shipPos;
    if (dot(shipDir, shipDir) < shipRadius * shipRadius) {
        // Destroyed by ship! Respawn particle at the top of the screen with a blast
        float rVal = rand(vec2(float(index), time));
        particles[index].x = rVal * 1600.0;
        particles[index].y = -20.0; // Above screen
        particles[index].vx = (rVal - 0.5) * 800.0; // Blast outwards
        particles[index].vy = 500.0 + rVal * 500.0; // Blast downwards
        
        // Flash bright white when spawned
        particles[index].r = 1.0;
        particles[index].g = 1.0;
        particles[index].b = 1.0;
        particles[index].a = 1.0;
        return;
    }

    // --- GRAVITY & PHYSICS ---
    vec2 dir = attractor - p;
    float distSq = dot(dir, dir);
    distSq = clamp(distSq, 50.0, 800000.0); // Prevent singularity
    
    float force = 0.0;
    // Only apply attraction if LMB is held, otherwise they just drift
    if (attractForce > 0.5) force = (10000000.0 * forceMultiplier) / distSq;
    if (repel > 0.5) force = -(15000000.0 * forceMultiplier) / distSq;

    v += normalize(dir) * force * dt;
    
    // Swirling effect around the mouse
    if (force != 0.0) {
        vec2 tangent = vec2(-dir.y, dir.x);
        float orbitFactor = (repel > 0.5) ? -0.1 : 0.6;
        v += normalize(tangent) * (abs(force) * orbitFactor) * dt;
    }

    // Light friction so they remain dynamic
    v *= 0.995; 
    p += v * dt;

    // Hard bounce off walls to keep them on screen
    if (p.x < 0.0) { v.x *= -0.8; p.x = 0.0; }
    if (p.x > 1600.0) { v.x *= -0.8; p.x = 1600.0; }
    if (p.y < 0.0) { v.y *= -0.8; p.y = 0.0; }
    if (p.y > 900.0) { v.y *= -0.8; p.y = 900.0; }

    particles[index].x = p.x;
    particles[index].y = p.y;
    particles[index].vx = v.x;
    particles[index].vy = v.y;
    
    // --- DYNAMIC COLOR MAPPING ---
    float speed = length(v);
    float normSpeed = clamp(speed * 0.005, 0.0, 1.0); // Scale speed for color
    
    if (colorMode == 0) {
        // Plasma Fire (Bright and saturated)
        particles[index].r = clamp(normSpeed * 1.5 + 0.4, 0.4, 1.0);
        particles[index].g = clamp(0.8 - normSpeed * 0.5, 0.2, 0.9);
        particles[index].b = clamp(0.4 - normSpeed * 0.2, 0.1, 0.6);
    } else if (colorMode == 1) {
        // Cyberpunk Neon
        particles[index].r = clamp(0.3 + normSpeed, 0.2, 1.0);
        particles[index].g = clamp(1.0 - normSpeed * 1.5, 0.1, 0.9);
        particles[index].b = clamp(0.9 + normSpeed * 0.5, 0.5, 1.0);
    } else {
        // Cosmic Aurora
        particles[index].r = clamp(sin(time * 0.5 + p.x * 0.002) * 0.5 + 0.5 + normSpeed, 0.3, 1.0);
        particles[index].g = clamp(cos(time * 0.3 + p.y * 0.002) * 0.5 + 0.5 - normSpeed*0.5, 0.3, 1.0);
        particles[index].b = clamp(sin(time * 0.7) * 0.5 + 0.8, 0.6, 1.0);
    }
    
    // Base alpha is much higher now so they never go completely black/invisible
    particles[index].a = clamp(speed * 0.002 + 0.4, 0.4, 1.0); 
}
)";

// Vertex Shader: Reads directly from the SSBO buffer as a VBO
const char* vertexShaderSrc = R"(
#version 430 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aVel;
layout(location = 2) in vec4 aColor;

out vec4 fragColor;
uniform mat4 mvp;

void main() {
    fragColor = aColor;
    gl_Position = mvp * vec4(aPos, 0.0, 1.0);
}
)";

// Fragment Shader
const char* fragmentShaderSrc = R"(
#version 430 core
in vec4 fragColor;
out vec4 finalColor;
void main() {
    finalColor = fragColor;
}
)";

unsigned int CompileShaderCustom(const char* source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        TraceLog(LOG_ERROR, "SHADER COMPILE ERROR: %s", infoLog);
    }
    return shader;
}

int main() {
    // Enable MSAA and set window size
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "GPU Compute Shader Particles (2 Million)");
    SetTargetFPS(0); // Uncapped FPS to benchmark GPU

    // Initialize 2 Million Particles on CPU (Only done once)
    std::vector<Particle> particles(numParticles);
    for (int i = 0; i < numParticles; i++) {
        particles[i].x = GetRandomValue(0, screenWidth);
        particles[i].y = GetRandomValue(0, screenHeight);
        particles[i].vx = GetRandomValue(-500, 500); // Give them a big initial push
        particles[i].vy = GetRandomValue(-500, 500);
        particles[i].r = 1.0f;
        particles[i].g = 1.0f;
        particles[i].b = 1.0f;
        particles[i].a = 1.0f;
    }

    // Compile Compute Shader
    unsigned int compShader = CompileShaderCustom(computeShaderSrc, GL_COMPUTE_SHADER);
    unsigned int computeProgram = glCreateProgram();
    glAttachShader(computeProgram, compShader);
    glLinkProgram(computeProgram);

    // Compile Render Shader
    unsigned int vertShader = CompileShaderCustom(vertexShaderSrc, GL_VERTEX_SHADER);
    unsigned int fragShader = CompileShaderCustom(fragmentShaderSrc, GL_FRAGMENT_SHADER);
    unsigned int renderProgram = glCreateProgram();
    glAttachShader(renderProgram, vertShader);
    glAttachShader(renderProgram, fragShader);
    glLinkProgram(renderProgram);

    // Create VAO and SSBO
    unsigned int vao, ssbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a Shader Storage Buffer Object (SSBO)
    // This allows the Compute Shader to read/write, AND the Vertex Shader to read it as a VBO!
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // Setup Vertex Attributes (sharing the SSBO as a VBO)
    glBindBuffer(GL_ARRAY_BUFFER, ssbo);
    
    // aPos (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    // aVel (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(2 * sizeof(float)));
    // aColor (vec4)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(4 * sizeof(float)));

    glBindVertexArray(0);

    // Get Uniform Locations
    int attractorLoc = glGetUniformLocation(computeProgram, "attractor");
    int dtLoc = glGetUniformLocation(computeProgram, "dt");
    int repelLoc = glGetUniformLocation(computeProgram, "repel");
    int attractForceLoc = glGetUniformLocation(computeProgram, "attractForce");
    int forceMultLoc = glGetUniformLocation(computeProgram, "forceMultiplier");
    int timeLoc = glGetUniformLocation(computeProgram, "time");
    int colorModeLoc = glGetUniformLocation(computeProgram, "colorMode");
    
    int shipPosLoc = glGetUniformLocation(computeProgram, "shipPos");
    int shipRadiusLoc = glGetUniformLocation(computeProgram, "shipRadius");
    
    int mvpLoc = glGetUniformLocation(renderProgram, "mvp");

    float forceMultiplier = 1.0f;
    int colorMode = 0;
    float time = 0.0f;

    // Enemy War-Ship variables
    Vector2 shipPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    Vector2 shipVel = { 400.0f, 300.0f };
    float shipRadius = 80.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // Cap dt to prevent physics explosions
        if (dt < 0.001f) dt = 0.001f; // Prevent zero-time freezes
        time += dt;
        
        Vector2 mouse = GetMousePosition();
        
        // Interactivity
        float repel = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? 1.0f : 0.0f;
        float attractForce = IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? 1.0f : 0.0f;

        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            forceMultiplier += wheel * 0.5f;
            if (forceMultiplier < 0.1f) forceMultiplier = 0.1f;
            if (forceMultiplier > 5.0f) forceMultiplier = 5.0f;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            colorMode = (colorMode + 1) % 3;
        }

        // --- ENEMY SHIP UPDATE ---
        shipPos.x += shipVel.x * dt;
        shipPos.y += shipVel.y * dt;
        if (shipPos.x < shipRadius || shipPos.x > screenWidth - shipRadius) shipVel.x *= -1.0f;
        if (shipPos.y < shipRadius || shipPos.y > screenHeight - shipRadius) shipVel.y *= -1.0f;

        // --- 1. RUN COMPUTE SHADER (GPU Physics) ---
        glUseProgram(computeProgram);
        glUniform2f(attractorLoc, mouse.x, mouse.y);
        glUniform1f(dtLoc, dt);
        glUniform1f(repelLoc, repel);
        glUniform1f(attractForceLoc, attractForce);
        glUniform1f(forceMultLoc, forceMultiplier);
        glUniform1f(timeLoc, time);
        glUniform1i(colorModeLoc, colorMode);
        
        glUniform2f(shipPosLoc, shipPos.x, shipPos.y);
        glUniform1f(shipRadiusLoc, shipRadius);

        // Dispatch compute (2,000,000 / 256 = 7813 work groups)
        glDispatchCompute((numParticles + 255) / 256, 1, 1);
        
        // Wait for compute to finish writing to the buffer before we draw from it
        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // --- 2. RENDER (GPU Drawing) ---
        BeginDrawing();
        ClearBackground(Color{ 5, 5, 10, 255 });

        rlDrawRenderBatchActive(); // Flush Raylib's internal batch

        glUseProgram(renderProgram);
        
        // Get Raylib MVP matrix for 2D screen coordinates
        Matrix matProjection = rlGetMatrixProjection();
        Matrix matModelview = rlGetMatrixModelview();
        Matrix mvp = MatrixMultiply(matModelview, matProjection);
        
        // Convert Raylib matrix to OpenGL float array (column-major)
        float mvpArray[16] = {
            mvp.m0, mvp.m4, mvp.m8, mvp.m12,
            mvp.m1, mvp.m5, mvp.m9, mvp.m13,
            mvp.m2, mvp.m6, mvp.m10, mvp.m14,
            mvp.m3, mvp.m7, mvp.m11, mvp.m15
        };
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpArray);

        // Additive blending for glowing particles
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // Draw 2 Million Points instantly
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, numParticles);
        glBindVertexArray(0);
        
        // Reset state so standard Raylib functions work again
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(0);
        rlDisableVertexArray();

        // --- 3. DRAW ENEMY WAR-SHIP (CPU Side Raylib Draw) ---
        // A glowing red forcefield
        DrawCircleGradient((int)shipPos.x, (int)shipPos.y, shipRadius, Fade(RED, 0.5f), BLANK);
        DrawCircleLines((int)shipPos.x, (int)shipPos.y, shipRadius, RED);
        
        // The spinning core of the ship
        DrawPoly(shipPos, 3, 40.0f, time * 100.0f, MAROON);
        DrawPoly(shipPos, 3, 25.0f, -time * 150.0f, RED);
        DrawPoly(shipPos, 6, 15.0f, time * 200.0f, ORANGE);
        DrawText("ENEMY WAR-SHIP", (int)shipPos.x - 45, (int)shipPos.y - (int)shipRadius - 20, 10, RED);

        // --- 4. UI (Standard Raylib) ---
        DrawRectangle(10, 10, 360, 170, Fade(BLACK, 0.8f));
        DrawText("ADVANCED COMPUTE SHADER", 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Particles: %d (2 Million!)", numParticles), 20, 50, 16, GREEN);
        DrawText(TextFormat("FPS: %d", GetFPS()), 20, 70, 16, YELLOW);
        DrawText(TextFormat("Gravity Force: %.1fx (Scroll to change)", forceMultiplier), 20, 95, 14, SKYBLUE);
        
        const char* colorName = "Plasma Fire";
        if (colorMode == 1) colorName = "Cyberpunk Neon";
        if (colorMode == 2) colorName = "Cosmic Aurora";
        DrawText(TextFormat("Color Mode: %s (Space to switch)", colorName), 20, 115, 14, PURPLE);
        
        DrawText("Hold LMB: Gravity Well | Hold RMB: Supernova", 20, 140, 14, LIGHTGRAY);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
