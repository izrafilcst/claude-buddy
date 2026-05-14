#pragma once
#include <TFT_eSPI.h>

enum class MascotState : uint8_t {
    RADIANT,   // quota >= 80%
    CONTENT,   // 60–79%
    NORMAL,    // 40–59%
    TIRED,     // 20–39%
    EXHAUSTED, // 5–19%
    OFF,       // 0%
    CONFUSED,  // agent offline
    SEARCHING  // WiFi disconnected
};

struct Particle {
    float x, y, vx, vy;
    uint8_t life;
};

class Tamagotchi {
public:
    void begin(TFT_eSPI* tft);
    void setState(int quota_percent, bool agent_online, bool wifi_connected);
    void drawFrame();
    MascotState state() const { return _state; }

private:
    TFT_eSPI*    _tft    = nullptr;
    TFT_eSprite* _sprite = nullptr;
    MascotState  _state  = MascotState::NORMAL;

    static constexpr int SPR          = 64;
    static constexpr int CLAWD_BASE_Y = 150;
    static constexpr int MAX_PARTICLES = 6;

    // ── Claw'd geometry inside the 64×64 sprite ─────────────────────────
    // Body: 44×22 px, vertically offset to leave room for legs below
    static constexpr int B_W  = 44;
    static constexpr int B_H  = 22;
    static constexpr int B_L  = (SPR - B_W) / 2;   // 10
    static constexpr int B_R  = B_L + B_W;          // 54
    static constexpr int B_T  = 6;
    static constexpr int B_B  = B_T + B_H;          // 28
    // Legs: 4 × (8×14 px), evenly spaced below body
    static constexpr int L_W  = 8;
    static constexpr int L_H  = 14;
    // Eyes: 8×8 px squares, in the upper portion of the body
    static constexpr int E_SZ = 8;
    static constexpr int E_Y  = B_T + 4;            // 10
    static constexpr int EL_X = B_L + 4;            // 14  (left eye)
    static constexpr int ER_X = B_R - E_SZ - 4;     // 42  (right eye)

    float   _bounce_y  = 0;
    float   _tremor_x  = 0;
    uint8_t _zzz_frame = 0;
    uint8_t _blink     = 0;   // 0–3 = closed, 4–149 = open, wraps

    Particle _particles[MAX_PARTICLES] = {};
    struct { int x, y, w, h; } _dirty = {};

    MascotState _quotaToState(int pct);
    void _updateAnim();
    void _drawClawd();
    void _drawEyes();
    void _drawParticles(int cx, int cy);
    void _drawZzz(int cx, int cy);
};
