#include "simulation.hpp"
#include "raylib.h"

#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <algorithm>

namespace {

constexpr int kSimWidth = 1220;
constexpr int kSimHeight = 640;
constexpr int kSampleRate = 44100;
constexpr int kSampleSize = 16;
constexpr int kChannels = 1;

// --- Audio Synthesis Logic ---
// We use global variables for the audio callback because Raylib's
// SetAudioStreamCallback doesn't easily support capturing member variables or lambdas without static state.
float g_frequency = 0.0f;
float g_phase = 0.0f;
float g_volume = 0.0f;
int g_waveformType = 0; // 0=Sine, 1=Square, 2=Sawtooth

void AudioInputCallback(void* buffer, unsigned int frames) {
    short* d = (short*)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        // Calculate the current sample value based on waveform type
        float sample = 0.0f;
        
        if (g_frequency > 0.0f) {
            if (g_waveformType == 0) {
                // Sine wave
                sample = sinf(2.0f * PI * g_phase);
            } else if (g_waveformType == 1) {
                // Square wave
                sample = (g_phase < 0.5f) ? 1.0f : -1.0f;
            } else if (g_waveformType == 2) {
                // Sawtooth wave
                sample = 2.0f * g_phase - 1.0f;
            }

            g_phase += g_frequency / kSampleRate;
            if (g_phase > 1.0f) g_phase -= 1.0f;
        }

        // Apply volume and convert to 16-bit integer
        d[i] = (short)(sample * 32000.0f * g_volume);
    }
}

// --- Synth Simulation Class ---
struct NoteEvent {
    float time;
    float frequency;
    bool isNoteOn;
};

class SynthSimulation : public Simulation {
public:
    SynthSimulation() {
        if (!IsAudioDeviceReady()) {
            InitAudioDevice();
        }
        
        SetAudioStreamBufferSizeDefault(4096);
        stream = LoadAudioStream(kSampleRate, kSampleSize, kChannels);
        SetAudioStreamCallback(stream, AudioInputCallback);
        PlayAudioStream(stream);
        
        reset();
    }

    ~SynthSimulation() override {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
        // Note: We don't CloseAudioDevice() here because other parts of the app might use it,
        // and Raylib usually handles it on window close.
    }

    const char* name() const override { return "Synth & Sequencer"; }

    void reset() override {
        g_frequency = 0.0f;
        g_volume = 0.0f;
        g_waveformType = 0;
        
        recordedNotes.clear();
        isRecording = false;
        isPlaying = false;
        recordingStartTime = 0.0f;
        playbackStartTime = 0.0f;
        playbackIndex = 0;
        
        activeKey = -1;
    }

    void update(float deltaTime) override {
        // Must be called every frame to keep the stream fed
        if (IsAudioStreamProcessed(stream)) {
            // Stream is hungry, but our callback feeds it automatically. 
            // We just need to ensure the stream stays active.
        }

        // Handle Waveform switching
        if (IsKeyPressed(KEY_UP)) { g_waveformType = (g_waveformType + 1) % 3; }
        if (IsKeyPressed(KEY_DOWN)) { g_waveformType = (g_waveformType + 2) % 3; }

        // Handle Sequencer Controls
        if (IsKeyPressed(KEY_SPACE)) {
            if (isRecording) {
                isRecording = false; // Stop recording
                StopNote();
            } else if (isPlaying) {
                isPlaying = false; // Stop playback
                StopNote();
            } else {
                // Start Playback if we have notes, otherwise Start Recording
                if (!recordedNotes.empty()) {
                    isPlaying = true;
                    playbackStartTime = GetTime();
                    playbackIndex = 0;
                } else {
                    isRecording = true;
                    recordingStartTime = GetTime();
                    recordedNotes.clear();
                }
            }
        }
        
        if (IsKeyPressed(KEY_BACKSPACE)) {
            recordedNotes.clear();
            isRecording = false;
            isPlaying = false;
            StopNote();
        }
        
        if (IsKeyPressed(KEY_R)) {
            if (!isPlaying) {
                isRecording = !isRecording;
                if (isRecording) {
                    recordingStartTime = GetTime();
                    recordedNotes.clear();
                } else {
                    StopNote();
                }
            }
        }

        // Handle Playback Logic
        if (isPlaying) {
            float currentTime = GetTime() - playbackStartTime;
            while (playbackIndex < recordedNotes.size() && recordedNotes[playbackIndex].time <= currentTime) {
                if (recordedNotes[playbackIndex].isNoteOn) {
                    PlayNoteFreq(recordedNotes[playbackIndex].frequency);
                } else {
                    StopNote();
                }
                playbackIndex++;
            }
            if (playbackIndex >= recordedNotes.size()) {
                isPlaying = false; // Finished sequence
                StopNote();
            }
            return; // Don't process manual keys while playing
        }

        // --- Keyboard to Piano Mapping ---
        // A,S,D,F,G,H,J,K,L = C4, D4, E4, F4, G4, A4, B4, C5, D5
        // W,E, T,Y,U = C#4, D#4, F#4, G#4, A#4
        
        bool keyPressedThisFrame = false;
        
        // Note: Using a simple hardcoded mapping for demonstration
        if (CheckKey(KEY_A, 261.63f)) keyPressedThisFrame = true; // C4
        else if (CheckKey(KEY_W, 277.18f)) keyPressedThisFrame = true; // C#4
        else if (CheckKey(KEY_S, 293.66f)) keyPressedThisFrame = true; // D4
        else if (CheckKey(KEY_E, 311.13f)) keyPressedThisFrame = true; // D#4
        else if (CheckKey(KEY_D, 329.63f)) keyPressedThisFrame = true; // E4
        else if (CheckKey(KEY_F, 349.23f)) keyPressedThisFrame = true; // F4
        else if (CheckKey(KEY_T, 369.99f)) keyPressedThisFrame = true; // F#4
        else if (CheckKey(KEY_G, 392.00f)) keyPressedThisFrame = true; // G4
        else if (CheckKey(KEY_Y, 415.30f)) keyPressedThisFrame = true; // G#4
        else if (CheckKey(KEY_H, 440.00f)) keyPressedThisFrame = true; // A4
        else if (CheckKey(KEY_U, 466.16f)) keyPressedThisFrame = true; // A#4
        else if (CheckKey(KEY_J, 493.88f)) keyPressedThisFrame = true; // B4
        else if (CheckKey(KEY_K, 523.25f)) keyPressedThisFrame = true; // C5
        else if (CheckKey(KEY_L, 587.33f)) keyPressedThisFrame = true; // D5

        if (!keyPressedThisFrame && activeKey != -1 && IsKeyReleased(activeKey)) {
            StopNote();
            activeKey = -1;
        }
    }

    void draw() const override {
        DrawRectangle(0, 0, kSimWidth, kSimHeight, Color{25, 30, 35, 255});

        // --- Draw UI ---
        DrawText("RETRO SYNTHESIZER & SEQUENCER", 30, 30, 30, RAYWHITE);
        
        const char* waveName = "Sine";
        if (g_waveformType == 1) waveName = "Square";
        if (g_waveformType == 2) waveName = "Sawtooth";
        DrawText(TextFormat("Waveform: %s (Up/Down Arrow)", waveName), 30, 80, 20, SKYBLUE);

        // State Indicator
        if (isRecording) {
            DrawText("RECORDING...", 30, 120, 20, RED);
        } else if (isPlaying) {
            DrawText("PLAYING...", 30, 120, 20, GREEN);
        } else {
            DrawText("IDLE", 30, 120, 20, GRAY);
        }

        // Instructions
        int infoX = kSimWidth - 400;
        DrawRectangle(infoX, 20, 380, 200, Fade(BLACK, 0.5f));
        DrawText("CONTROLS:", infoX + 20, 30, 20, GOLD);
        DrawText("Keys A-L: White Keys (C4 to D5)", infoX + 20, 70, 16, RAYWHITE);
        DrawText("Keys W,E,T,Y,U: Black Keys", infoX + 20, 95, 16, RAYWHITE);
        DrawText("R or SPACE: Start/Stop Recording", infoX + 20, 130, 16, RAYWHITE);
        DrawText("SPACE: Play Sequence (when idle)", infoX + 20, 155, 16, RAYWHITE);
        DrawText("BACKSPACE: Clear Sequence", infoX + 20, 180, 16, RED);

        // --- Draw Timeline ---
        DrawRectangle(30, 200, kSimWidth - 60, 100, Fade(BLACK, 0.8f));
        DrawRectangleLines(30, 200, kSimWidth - 60, 100, GRAY);
        DrawText("SEQUENCER TIMELINE", 40, 210, 14, GRAY);
        
        if (!recordedNotes.empty()) {
            float totalTime = recordedNotes.back().time;
            if (totalTime < 5.0f) totalTime = 5.0f; // Minimum view width
            
            float widthPerSec = (kSimWidth - 80) / totalTime;
            
            float currentNoteStart = -1.0f;
            float currentFreq = 0.0f;

            for (const auto& note : recordedNotes) {
                if (note.isNoteOn) {
                    currentNoteStart = note.time;
                    currentFreq = note.frequency;
                } else if (currentNoteStart >= 0.0f) {
                    float x = 40 + (currentNoteStart * widthPerSec);
                    float w = (note.time - currentNoteStart) * widthPerSec;
                    float y = 280 - (currentFreq - 200) * 0.1f; // Visual height based on pitch
                    DrawRectangle((int)x, (int)y, (int)std::max(2.0f, w), 10, SKYBLUE);
                    currentNoteStart = -1.0f;
                }
            }
            
            // Draw Playhead
            if (isPlaying) {
                float px = 40 + ((GetTime() - playbackStartTime) * widthPerSec);
                DrawLine((int)px, 200, (int)px, 300, RED);
            }
        }

        // --- Draw Piano Keys ---
        int startX = (kSimWidth - (9 * 60)) / 2;
        int startY = 400;
        
        // White Keys
        int whiteKeyCodes[] = {KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L};
        const char* whiteKeyLabels[] = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
        for (int i = 0; i < 9; i++) {
            Rectangle rec = {(float)startX + i * 62, (float)startY, 60, 200};
            bool isPressed = IsKeyDown(whiteKeyCodes[i]);
            DrawRectangleRec(rec, isPressed ? LIGHTGRAY : RAYWHITE);
            DrawRectangleLinesEx(rec, 2, BLACK);
            DrawText(whiteKeyLabels[i], startX + i * 62 + 25, startY + 170, 20, isPressed ? BLACK : GRAY);
        }

        // Black Keys
        int blackOffsets[] = {1, 2, 4, 5, 6}; // Positions relative to white keys
        int blackKeyCodes[] = {KEY_W, KEY_E, KEY_T, KEY_Y, KEY_U};
        const char* blackKeyLabels[] = {"W", "E", "T", "Y", "U"};
        
        for (int i = 0; i < 5; i++) {
            Rectangle rec = {(float)startX + blackOffsets[i] * 62 - 20, (float)startY, 40, 120};
            bool isPressed = IsKeyDown(blackKeyCodes[i]);
            DrawRectangleRec(rec, isPressed ? DARKGRAY : BLACK);
            DrawRectangleLinesEx(rec, 2, GRAY);
            DrawText(blackKeyLabels[i], startX + blackOffsets[i] * 62 - 5, startY + 90, 16, isPressed ? BLACK : RAYWHITE);
        }
    }

private:
    bool CheckKey(int key, float frequency) {
        if (IsKeyPressed(key)) {
            PlayNoteFreq(frequency);
            activeKey = key;
            return true;
        }
        if (IsKeyDown(key)) {
            return true;
        }
        return false;
    }

    void PlayNoteFreq(float frequency) {
        g_frequency = frequency;
        g_volume = 0.5f; // Hardcoded volume for now
        
        if (isRecording) {
            recordedNotes.push_back({ (float)(GetTime() - recordingStartTime), frequency, true });
        }
    }

    void StopNote() {
        g_volume = 0.0f;
        
        if (isRecording) {
             recordedNotes.push_back({ (float)(GetTime() - recordingStartTime), 0.0f, false });
        }
    }

    AudioStream stream;
    
    std::vector<NoteEvent> recordedNotes;
    bool isRecording;
    bool isPlaying;
    double recordingStartTime;
    double playbackStartTime;
    size_t playbackIndex;
    
    int activeKey;
};

} // namespace

std::unique_ptr<Simulation> CreateSynthSimulation() {
    return std::make_unique<SynthSimulation>();
}
