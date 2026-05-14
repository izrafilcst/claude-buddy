#!/usr/bin/env python3
"""Generate mascot-beach.gif — Claude Buddy orb on a beach, day→night cycle."""
from PIL import Image, ImageDraw, ImageFilter
import math, os

W, H = 480, 220
FRAMES = 48
DURATION = 80

def lerp(a, b, t):
    t = max(0.0, min(1.0, t))
    return a + (b - a) * t

def lerp_rgb(c1, c2, t):
    return tuple(int(lerp(c1[i], c2[i], t)) for i in range(3))

SKY_DAY     = (135, 210, 245)
SKY_GOLDEN  = (255, 175,  90)
SKY_DUSK    = (120,  60, 110)
SKY_NIGHT   = ( 12,  18,  48)
OCEAN_DAY   = ( 55, 160, 220)
OCEAN_NIGHT = (  8,  25,  75)
SAND_LIT    = (230, 205, 155)
SAND_DARK   = (140, 120,  80)
ORB_CORE    = (193,  95,  60)
ORB_LIGHT   = (240, 145, 100)
ORB_GLOW    = (232, 132,  92)

HORIZON_Y   = int(H * 0.56)
SAND_Y      = int(H * 0.70)

def sky_color(t):
    # t: 0=day, 0.4=sunset, 0.55=night, 0.85=pre-dawn, 1=day
    if t < 0.35:
        return lerp_rgb(SKY_DAY, SKY_GOLDEN, t / 0.35)
    elif t < 0.50:
        return lerp_rgb(SKY_GOLDEN, SKY_DUSK, (t - 0.35) / 0.15)
    elif t < 0.60:
        return lerp_rgb(SKY_DUSK, SKY_NIGHT, (t - 0.50) / 0.10)
    elif t < 0.85:
        return SKY_NIGHT
    else:
        return lerp_rgb(SKY_NIGHT, SKY_DAY, (t - 0.85) / 0.15)

def ocean_color(t):
    night_t = max(0, min(1, (t - 0.40) / 0.20)) if t < 0.85 else lerp(1, 0, (t - 0.85) / 0.15)
    return lerp_rgb(OCEAN_DAY, OCEAN_NIGHT, night_t)

def sand_color(t):
    night_t = max(0, min(1, (t - 0.45) / 0.15)) if t < 0.85 else lerp(1, 0, (t - 0.85) / 0.15)
    return lerp_rgb(SAND_LIT, SAND_DARK, night_t)

STARS = [
    (40, 18), (85, 8), (140, 30), (190, 12), (240, 35),
    (300, 20), (350, 10), (400, 38), (450, 22), (60, 45),
    (160, 50), (280, 48), (420, 14), (110, 55), (330, 52),
]

frames = []
for f in range(FRAMES):
    t = f / FRAMES

    sc = sky_color(t)
    oc = ocean_color(t)
    sac = sand_color(t)
    night_t = max(0, min(1, (t - 0.50) / 0.10)) if t < 0.85 else lerp(1, 0, (t - 0.85) / 0.15)

    img = Image.new('RGBA', (W, H), (*sc, 255))
    draw = ImageDraw.Draw(img)

    # Sky gradient (lighter at horizon)
    horizon_c = lerp_rgb(sc, (255, 255, 255), 0.15)
    for y in range(HORIZON_Y):
        blend = y / HORIZON_Y
        row_c = lerp_rgb(sc, horizon_c, blend * 0.4)
        draw.line([(0, y), (W, y)], fill=(*row_c, 255))

    # Stars
    for sx, sy in STARS:
        alpha = int(255 * night_t * (0.6 + 0.4 * math.sin(f * 0.3 + sx)))
        r = 1 if sx % 3 != 0 else 2
        draw.ellipse([sx - r, sy - r, sx + r, sy + r], fill=(255, 255, 220, alpha))

    # Moon
    if night_t > 0.05:
        ma = int(255 * night_t)
        mx, my = 390, 38
        draw.ellipse([mx - 18, my - 18, mx + 18, my + 18], fill=(245, 245, 210, ma))
        draw.ellipse([mx - 8, my - 22, mx + 14, my + 14], fill=(*sky_color(0.65), ma))

    # Sun
    sun_alpha_t = (1 - max(0, min(1, (t - 0.30) / 0.20))) if t < 0.5 else max(0, min(1, (t - 0.85) / 0.15))
    if sun_alpha_t > 0.05:
        angle = t * 2 * math.pi
        sun_x = int(W * 0.5 + W * 0.3 * math.cos(angle + math.pi))
        sun_y = int(HORIZON_Y * 0.5 - HORIZON_Y * 0.3 * math.sin(angle))
        sun_y = max(15, min(HORIZON_Y - 10, sun_y))
        sa = int(255 * sun_alpha_t)
        # glow
        for gr in [28, 22, 18]:
            ga = int(sa * 0.25)
            draw.ellipse([sun_x - gr, sun_y - gr, sun_x + gr, sun_y + gr],
                         fill=(255, 220, 80, ga))
        draw.ellipse([sun_x - 14, sun_y - 14, sun_x + 14, sun_y + 14],
                     fill=(255, 230, 60, sa))

    # Ocean
    draw.rectangle([0, HORIZON_Y, W, SAND_Y], fill=(*oc, 255))

    # Ocean shimmer
    shimmer_t = max(0, 1 - night_t * 0.7)
    for wx in range(0, W, 22):
        wave_x = (wx + f * 4) % W
        wy = HORIZON_Y + 8 + int(4 * math.sin(wave_x * 0.05 + f * 0.3))
        sc2 = lerp_rgb(oc, (255, 255, 255), 0.25 * shimmer_t)
        draw.ellipse([wave_x, wy, wave_x + 18, wy + 4], outline=(*sc2, 180), width=1)

    # Sand
    draw.rectangle([0, SAND_Y, W, H], fill=(*sac, 255))

    # Sand texture lines
    for sy_off in range(0, H - SAND_Y, 12):
        y2 = SAND_Y + sy_off
        sc3 = lerp_rgb(sac, (0, 0, 0), 0.05)
        draw.line([(0, y2), (W, y2)], fill=(*sc3, 60), width=1)

    # Orb mascot — sitting on sand
    pulse = 0.90 + 0.10 * math.sin(f * 0.45)
    orb_r = int(32 * pulse)
    orb_x = W // 2
    orb_y = SAND_Y - orb_r + 6

    # Glow (multi-layer)
    glow_img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow_img)
    for layer, (gr, ga_base) in enumerate([(orb_r + 28, 18), (orb_r + 18, 35), (orb_r + 10, 55)]):
        ga = int(ga_base * (0.5 + 0.5 * night_t + 0.3))
        gd.ellipse([orb_x - gr, orb_y - gr, orb_x + gr, orb_y + gr],
                   fill=(*ORB_GLOW, ga))
    img = Image.alpha_composite(img, glow_img)
    draw = ImageDraw.Draw(img)

    # Core
    draw.ellipse([orb_x - orb_r, orb_y - orb_r, orb_x + orb_r, orb_y + orb_r],
                 fill=(*ORB_CORE, 255))

    # Radial gradient (lighter center-top)
    for gr in range(orb_r, 0, -4):
        ratio = 1 - gr / orb_r
        rc = lerp_rgb(ORB_CORE, ORB_LIGHT, ratio * 0.6)
        draw.ellipse([orb_x - gr, orb_y - orb_r + 4, orb_x + gr, orb_y - orb_r + 4 + gr * 2],
                     fill=(*rc, 80))

    # Specular highlight
    hl_r = orb_r // 3
    draw.ellipse([orb_x - hl_r * 2, orb_y - orb_r + 4,
                  orb_x - hl_r // 2, orb_y - orb_r + 4 + hl_r],
                 fill=(255, 215, 185, 160))

    # Eyes (blinking on frame 0 and 24)
    blink = f in (0, 1, 24, 25)
    eye_y = orb_y - 4
    if blink:
        draw.line([(orb_x - 9, eye_y), (orb_x - 3, eye_y)], fill=(40, 15, 8), width=2)
        draw.line([(orb_x + 3, eye_y), (orb_x + 9, eye_y)], fill=(40, 15, 8), width=2)
    else:
        draw.ellipse([orb_x - 9, eye_y - 3, orb_x - 3, eye_y + 3], fill=(40, 15, 8))
        draw.ellipse([orb_x + 3, eye_y - 3, orb_x + 9, eye_y + 3], fill=(40, 15, 8))
        draw.ellipse([orb_x - 7, eye_y - 2, orb_x - 6, eye_y - 1], fill=(255, 255, 255))
        draw.ellipse([orb_x + 5, eye_y - 2, orb_x + 6, eye_y - 1], fill=(255, 255, 255))

    # Shadow on sand
    shad_alpha = int(50 + 30 * night_t)
    draw.ellipse([orb_x - orb_r + 6, SAND_Y + 2, orb_x + orb_r - 6, SAND_Y + 10],
                 fill=(80, 60, 30, shad_alpha))

    frames.append(img.convert('RGB'))

out = os.path.join(os.path.dirname(__file__), 'mascot-beach.gif')
frames[0].save(
    out,
    save_all=True,
    append_images=frames[1:],
    loop=0,
    duration=DURATION,
    optimize=True,
)
print(f"✓ {FRAMES} frames → {out}")
