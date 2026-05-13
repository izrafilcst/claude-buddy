#pragma once
#include <TFT_eSPI.h>

enum class MascotState : uint8_t {
    RADIANT,   // 80-100%
    CONTENT,   // 60-79%
    NORMAL,    // 40-59%
    TIRED,     // 20-39%
    EXHAUSTED, // 5-19%
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
    TFT_eSPI* _tft = nullptr;
    TFT_eSprite* _sprite = nullptr;
    MascotState _state = MascotState::NORMAL;

    static constexpr int ORB_R = 28;
    static constexpr int SPR = 64;
    static constexpr int ORB_BASE_Y = 140;
    static constexpr int MAX_PARTICLES = 6;

    float _bounce_y = 0;
    float _pulse = 1.0f;
    float _tremor_x = 0;
    uint8_t _zzz_frame = 0;

    Particle _particles[MAX_PARTICLES] = {};
    struct { int x, y, w, h; } _dirty = {};

    MascotState _quotaToState(int pct);
    void _updateAnim();
    void _drawOrb();
    void _drawDots();
    void _drawParticles();
    void _drawZzz();
};
