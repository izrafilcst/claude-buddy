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
    if (pct <= 0)  return MascotState::OFF;
    if (pct < 5)   return MascotState::OFF;
    if (pct < 20)  return MascotState::EXHAUSTED;
    if (pct < 40)  return MascotState::TIRED;
    if (pct < 60)  return MascotState::NORMAL;
    if (pct < 80)  return MascotState::CONTENT;
    return MascotState::RADIANT;
}

void Tamagotchi::_updateAnim() {
    float t = millis() * 0.001f;
    _tremor_x = 0;

    switch (_state) {
    case MascotState::RADIANT:
        _bounce_y = sinf(t * 3.0f) * 15.0f;
        _pulse = 1.0f;
        break;
    case MascotState::CONTENT:
        _bounce_y = sinf(t * 2.0f) * 8.0f;
        _pulse = 1.0f;
        break;
    case MascotState::NORMAL:
        _bounce_y = 0;
        _pulse = 1.0f + sinf(t * 1.5f) * 0.05f;
        break;
    case MascotState::TIRED:
        _bounce_y = 30.0f;
        _tremor_x = (float)random(-2, 3);
        _pulse = 1.0f;
        break;
    case MascotState::EXHAUSTED:
        _bounce_y = 60.0f;
        _pulse = 0.9f;
        _zzz_frame++;
        break;
    case MascotState::OFF:
        _bounce_y = 60.0f;
        _pulse = 1.0f;
        break;
    case MascotState::CONFUSED:
        _bounce_y = sinf(t * 4.0f) * 3.0f;
        _pulse = 1.0f;
        break;
    case MascotState::SEARCHING:
        _bounce_y = sinf(t * 1.0f) * 20.0f;
        _pulse = 1.0f + sinf(t * 2.0f) * 0.03f;
        break;
    }
}

void Tamagotchi::_drawOrb() {
    int cx = SPR / 2, cy = SPR / 2;
    int r = (int)(ORB_R * _pulse);

    uint16_t base  = (_state == MascotState::OFF) ? Colors::GRAY : Colors::CRAIL;
    uint16_t light = (_state == MascotState::OFF) ? Colors::CLOUDY : Colors::CRAIL_LIGHT;

    if (_state == MascotState::EXHAUSTED) {
        base  = Colors::lerp(base, Colors::GRAY, 0.4f);
        light = Colors::lerp(light, Colors::GRAY, 0.4f);
    }

    if (_state == MascotState::RADIANT) {
        uint16_t glow = Colors::lerp(Colors::CRAIL_LIGHT, Colors::PAMPAS, 0.6f);
        _sprite->fillCircle(cx, cy, r + 4, glow);
    }

    for (int i = r; i > 0; i--) {
        float frac = 1.0f - ((float)i / r);
        _sprite->fillCircle(cx, cy, i, Colors::lerp(base, light, frac * frac));
    }
}

void Tamagotchi::_drawDots() {
    int cx = SPR / 2, cy = SPR / 2;
    float speed;
    switch (_state) {
    case MascotState::RADIANT: speed = 5.0f; break;
    case MascotState::CONTENT: speed = 3.0f; break;
    case MascotState::NORMAL:  speed = 2.0f; break;
    case MascotState::TIRED:   speed = 1.0f; break;
    default: speed = 1.5f; break;
    }
    float time = millis() * 0.001f * speed;
    int sp = 10;

    for (int i = 0; i < 3; i++) {
        int dx = cx + (i - 1) * sp;
        int dy = cy;

        switch (_state) {
        case MascotState::TIRED:
            _sprite->drawFastHLine(dx - 3, dy, 6, TFT_WHITE);
            break;
        case MascotState::OFF:
            _sprite->drawLine(dx - 2, dy - 2, dx + 2, dy + 2, TFT_WHITE);
            _sprite->drawLine(dx - 2, dy + 2, dx + 2, dy - 2, TFT_WHITE);
            break;
        case MascotState::CONFUSED:
            _sprite->setCursor(dx - 3, dy - 4);
            _sprite->setTextColor(TFT_WHITE);
            _sprite->print("?");
            break;
        default: {
            float phase = sinf(time + i * 2.094f);
            int r = 2 + (int)((phase + 1.0f) * 1.5f);
            _sprite->fillCircle(dx, dy, r, TFT_WHITE);
            break;
        }
        }
    }
}

void Tamagotchi::_drawParticles() {
    if (_state != MascotState::RADIANT) return;
    int ocx = SCREEN_W / 2 + (int)_tremor_x;
    int ocy = ORB_BASE_Y + (int)_bounce_y;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle& p = _particles[i];
        p.x += p.vx;
        p.y += p.vy;
        if (p.life > 0) p.life--;

        if (p.life == 0) {
            float a = random(0, 628) * 0.01f;
            p.x = ocx + cosf(a) * (ORB_R + 8);
            p.y = ocy + sinf(a) * (ORB_R + 8);
            p.vx = cosf(a) * 0.4f;
            p.vy = sinf(a) * 0.4f - 0.2f;
            p.life = random(20, 50);
        }

        int sz = (p.life > 25) ? 2 : 1;
        uint16_t c = (p.life > 15) ? Colors::CRAIL_LIGHT : Colors::CLOUDY;
        _tft->fillCircle((int)p.x, (int)p.y, sz, c);
    }
}

void Tamagotchi::_drawZzz() {
    int bx = SCREEN_W / 2 + ORB_R + 5;
    int by = ORB_BASE_Y + 40;
    int off = (_zzz_frame / 3) % 20;

    _tft->setTextColor(Colors::CLOUDY, Colors::PAMPAS);
    _tft->drawString("Z", bx, by - off, 2);
    _tft->drawString("z", bx + 8, by - off - 12, 1);
    _tft->drawString("z", bx + 14, by - off - 20, 1);
}

void Tamagotchi::drawFrame() {
    if (_dirty.w > 0) {
        _tft->fillRect(_dirty.x, _dirty.y, _dirty.w, _dirty.h, Colors::PAMPAS);
    }

    _updateAnim();

    int cx = SCREEN_W / 2 + (int)_tremor_x;
    int cy = ORB_BASE_Y + (int)_bounce_y;

    _sprite->fillSprite(Colors::TRANSPARENT);
    _drawOrb();
    _drawDots();

    int sx = cx - SPR / 2;
    int sy = cy - SPR / 2;
    _sprite->pushSprite(sx, sy, Colors::TRANSPARENT);

    _drawParticles();
    if (_state == MascotState::EXHAUSTED) _drawZzz();

    _dirty.x = sx - 20;
    _dirty.y = sy - 30;
    _dirty.w = SPR + 40;
    _dirty.h = SPR + 60;
}
