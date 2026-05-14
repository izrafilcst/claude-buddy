#include "tamagotchi.h"
#include "colors.h"
#include "config.h"

void Tamagotchi::begin(TFT_eSPI* tft) {
    _tft = tft;
    _sprite = new TFT_eSprite(tft);
    _sprite->setColorDepth(16);
    _sprite->createSprite(SPR, SPR);
    for (auto& p : _particles) p.life = 0;
}

void Tamagotchi::setState(int quota_percent, bool agent_online, bool wifi) {
    if (!wifi)              _state = MascotState::SEARCHING;
    else if (!agent_online) _state = MascotState::CONFUSED;
    else                    _state = _quotaToState(quota_percent);
}

MascotState Tamagotchi::_quotaToState(int pct) {
    if (pct < 5)  return MascotState::OFF;
    if (pct < 20) return MascotState::EXHAUSTED;
    if (pct < 40) return MascotState::TIRED;
    if (pct < 60) return MascotState::NORMAL;
    if (pct < 80) return MascotState::CONTENT;
    return MascotState::RADIANT;
}

void Tamagotchi::_updateAnim() {
    float t = millis() * 0.001f;

    switch (_state) {
    case MascotState::RADIANT:
        _bounce_y = sinf(t * 3.0f) * 8.0f;
        _tremor_x = 0;
        break;
    case MascotState::CONTENT:
        _bounce_y = sinf(t * 2.0f) * 5.0f;
        _tremor_x = 0;
        break;
    case MascotState::NORMAL:
        _bounce_y = 0;
        _tremor_x = 0;
        break;
    case MascotState::TIRED:
        _bounce_y = 14.0f;
        _tremor_x = (float)random(-1, 2);
        break;
    case MascotState::EXHAUSTED:
        _bounce_y = 24.0f;
        _tremor_x = 0;
        _zzz_frame++;
        break;
    case MascotState::OFF:
        _bounce_y = 20.0f;
        _tremor_x = 0;
        break;
    case MascotState::CONFUSED:
        _bounce_y = sinf(t * 5.0f) * 3.0f;
        _tremor_x = (float)random(-2, 3);
        break;
    case MascotState::SEARCHING:
        _bounce_y = sinf(t * 1.2f) * 10.0f;
        _tremor_x = 0;
        break;
    }

    // Blink: 4 frames closed (~264 ms), then 146 frames open (~9.6 s)
    if (++_blink >= 150) _blink = 0;
}

// ── Eyes ─────────────────────────────────────────────────────────────────────

void Tamagotchi::_drawEyes() {
    switch (_state) {

    case MascotState::EXHAUSTED:
    case MascotState::OFF: {
        // Closed: thick horizontal line across each eye slot
        int mid = E_Y + E_SZ / 2 - 1;
        _sprite->fillRect(EL_X, mid, E_SZ, 2, TFT_BLACK);
        _sprite->fillRect(ER_X, mid, E_SZ, 2, TFT_BLACK);
        break;
    }

    case MascotState::TIRED: {
        // Half-closed: only lower 4 px of each eye slot is black;
        // upper half is already body color from _drawClawd()
        int mid = E_Y + E_SZ / 2;
        _sprite->fillRect(EL_X, mid, E_SZ, E_SZ / 2, TFT_BLACK);
        _sprite->fillRect(ER_X, mid, E_SZ, E_SZ / 2, TFT_BLACK);
        break;
    }

    case MascotState::CONFUSED: {
        // X eyes
        _sprite->drawLine(EL_X + 1, E_Y + 1, EL_X + E_SZ - 2, E_Y + E_SZ - 2, TFT_BLACK);
        _sprite->drawLine(EL_X + E_SZ - 2, E_Y + 1, EL_X + 1, E_Y + E_SZ - 2, TFT_BLACK);
        _sprite->drawLine(ER_X + 1, E_Y + 1, ER_X + E_SZ - 2, E_Y + E_SZ - 2, TFT_BLACK);
        _sprite->drawLine(ER_X + E_SZ - 2, E_Y + 1, ER_X + 1, E_Y + E_SZ - 2, TFT_BLACK);
        break;
    }

    case MascotState::SEARCHING: {
        // Wide-open eyes, no blink — scanning the horizon
        _sprite->fillRect(EL_X, E_Y, E_SZ, E_SZ, TFT_BLACK);
        _sprite->fillRect(ER_X, E_Y, E_SZ, E_SZ, TFT_BLACK);
        _sprite->fillRect(EL_X + 1, E_Y + 1, 2, 2, Colors::WHITE);
        _sprite->fillRect(ER_X + 1, E_Y + 1, 2, 2, Colors::WHITE);
        break;
    }

    default: {
        // RADIANT / CONTENT / NORMAL — square eyes with periodic blink
        if (_blink < 4) {
            int mid = E_Y + E_SZ / 2 - 1;
            _sprite->fillRect(EL_X, mid, E_SZ, 2, TFT_BLACK);
            _sprite->fillRect(ER_X, mid, E_SZ, 2, TFT_BLACK);
        } else {
            _sprite->fillRect(EL_X, E_Y, E_SZ, E_SZ, TFT_BLACK);
            _sprite->fillRect(ER_X, E_Y, E_SZ, E_SZ, TFT_BLACK);
            // Shine dots
            _sprite->fillRect(EL_X + 1, E_Y + 1, 2, 2, Colors::WHITE);
            _sprite->fillRect(ER_X + 1, E_Y + 1, 2, 2, Colors::WHITE);
        }
        break;
    }
    }
}

// ── Body ─────────────────────────────────────────────────────────────────────

void Tamagotchi::_drawClawd() {
    uint16_t body_c = Colors::CRAIL;
    uint16_t shad_c = Colors::lerp(Colors::CRAIL, Colors::DARK_BG, 0.35f);

    if (_state == MascotState::OFF) {
        body_c = Colors::lerp(Colors::CRAIL, Colors::GRAY, 0.50f);
        shad_c = Colors::lerp(body_c, Colors::DARK_BG, 0.35f);
    } else if (_state == MascotState::EXHAUSTED) {
        body_c = Colors::lerp(Colors::CRAIL, Colors::GRAY, 0.28f);
        shad_c = Colors::lerp(body_c, Colors::DARK_BG, 0.35f);
    }

    // Soft halo behind the body for RADIANT state
    if (_state == MascotState::RADIANT) {
        uint16_t glow = Colors::lerp(Colors::CRAIL_LIGHT, Colors::PAMPAS, 0.55f);
        _sprite->fillRect(B_L - 5, B_T - 5, B_W + 10, B_H + 10, glow);
    }

    // Main body block
    _sprite->fillRect(B_L, B_T, B_W, B_H, body_c);
    // Shadow strip along the body bottom for depth
    _sprite->fillRect(B_L, B_B - 5, B_W, 5, shad_c);

    // Four legs — x positions 12, 23, 33, 44 (3-px gaps between legs)
    const int leg_xs[4] = { 12, 23, 33, 44 };
    for (int i = 0; i < 4; i++) {
        _sprite->fillRect(leg_xs[i], B_B, L_W, L_H, body_c);
        _sprite->fillRect(leg_xs[i], B_B + L_H - 4, L_W, 4, shad_c);
    }

    _drawEyes();
}

// ── Particles (RADIANT only, drawn directly on TFT) ──────────────────────────

void Tamagotchi::_drawParticles(int cx, int cy) {
    if (_state != MascotState::RADIANT) return;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = _particles[i];
        p.x += p.vx;
        p.y += p.vy;
        if (p.life > 0) p.life--;

        if (p.life == 0) {
            float a = random(0, 628) * 0.01f;
            float r = B_W / 2 + 10;
            p.x  = cx + cosf(a) * r;
            p.y  = cy + sinf(a) * r;
            p.vx = cosf(a) * 0.4f;
            p.vy = sinf(a) * 0.4f - 0.2f;
            p.life = random(20, 50);
        }

        int      sz = (p.life > 25) ? 2 : 1;
        uint16_t c  = (p.life > 15) ? Colors::CRAIL_LIGHT : Colors::CLOUDY;
        _tft->fillCircle((int)p.x, (int)p.y, sz, c);
    }
}

// ── Zzz (EXHAUSTED, drawn directly on TFT) ───────────────────────────────────

void Tamagotchi::_drawZzz(int cx, int cy) {
    // Upper-right of the body in screen coordinates
    int bx  = cx + B_W / 2 + 4;
    int by  = cy - SPR / 2 + B_T;   // = body-top row on screen
    int off = (_zzz_frame / 3) % 20;

    _tft->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    _tft->drawString("Z", bx,      by - off,      2);
    _tft->drawString("z", bx + 8,  by - off - 12, 1);
    _tft->drawString("z", bx + 14, by - off - 20, 1);
}

// ── Main draw call ────────────────────────────────────────────────────────────

void Tamagotchi::drawFrame() {
    if (_dirty.w > 0) {
        _tft->fillRect(_dirty.x, _dirty.y, _dirty.w, _dirty.h, Colors::PAMPAS);
    }

    _updateAnim();

    int cx = SCREEN_W / 2 + (int)_tremor_x;
    int cy = CLAWD_BASE_Y  + (int)_bounce_y;

    _sprite->fillSprite(Colors::TRANSPARENT);
    _drawClawd();

    int sx = cx - SPR / 2;
    int sy = cy - SPR / 2;
    _sprite->pushSprite(sx, sy, Colors::TRANSPARENT);

    _drawParticles(cx, cy);
    if (_state == MascotState::EXHAUSTED) _drawZzz(cx, cy);

    // Dirty rect covers sprite + particles (30 px margin) + rising Zzz text above
    _dirty = { sx - 30, sy - 35, SPR + 60, SPR + 65 };
}
