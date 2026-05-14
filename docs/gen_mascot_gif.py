#!/usr/bin/env python3
"""Generate mascot-beach.gif — Claw'd on a beach.
Day (~5s): fishing with animated bobber.
Night (~15s): sleeping with rising Zzz.
"""
from PIL import Image, ImageDraw
import math, os

W, H = 480, 220

DAY_FRAMES    = 50
NIGHT_FRAMES  = 75   # × 200ms = 15s
DUR_DAY       = 100  # ms
DUR_NIGHT     = 200  # ms

# Palette
SKY_DAY    = (210, 235, 252)
SKY_NIGHT  = ( 12,  18,  48)
OCEAN_DAY  = ( 55, 165, 220)
OCEAN_NIGHT= (  8,  22,  72)
SAND_DAY   = (232, 208, 158)
SAND_NIGHT = (148, 128,  88)
CLAWD      = (196,  88,  58)   # terracotta — matches reference
CLAWD_SHAD = (158,  65,  42)
EYE        = ( 38,  22,  12)
ROD_C      = ( 90,  58,  28)
LINE_C     = (190, 190, 190)
BOBBER_R   = (210,  55,  50)
BOBBER_W   = (245, 245, 245)
ZZZ_C      = (200, 215, 255)

HORIZON_Y  = int(H * 0.55)
SAND_Y     = int(H * 0.70)

# Claw'd geometry
CX       = 160
BODY_W   = 56
BODY_H   = 36
BODY_BOT = SAND_Y - 14
BODY_TOP = BODY_BOT - BODY_H
BODY_L   = CX - BODY_W // 2
BODY_R   = CX + BODY_W // 2

LEG_W, LEG_H = 10, 16
LEG_Y0 = BODY_BOT
LEG_Y1 = LEG_Y0 + LEG_H
LEG_XS = [BODY_L + 3, BODY_L + 15, BODY_R - 25, BODY_R - 13]

EYE_H2, EYE_W2 = 8, 10
EYE_Y  = BODY_TOP + 10
EYE_LX = CX - 16
EYE_RX = CX + 6

ROD_BX = BODY_R - 1
ROD_BY = BODY_TOP + 10
ROD_TX = ROD_BX + 90
ROD_TY = ROD_BY - 60

BOB_X  = ROD_TX + 28
BOB_Y0 = HORIZON_Y + 10

STARS = [
    (42,12),(88,7),(148,26),(198,8),(262,32),(322,16),
    (378,6),(438,28),(58,40),(168,46),(302,43),(418,10),
    (108,50),(342,48),(462,32),(20,24),(230,18),
]

def lerp_rgb(c1, c2, t):
    t = max(0.0, min(1.0, t))
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3))

def draw_sky_gradient(draw, top_c, hor_c):
    for y in range(HORIZON_Y):
        t = y / HORIZON_Y
        draw.line([(0, y), (W, y)], fill=lerp_rgb(top_c, hor_c, t))

def draw_ocean(draw, color):
    draw.rectangle([0, HORIZON_Y, W, SAND_Y], fill=color)

def draw_sand(draw, color):
    draw.rectangle([0, SAND_Y, W, H], fill=color)

def draw_clawd(draw, closed=False, bob=0):
    # Body
    draw.rectangle([BODY_L, BODY_TOP + bob, BODY_R, BODY_BOT + bob], fill=CLAWD)
    # Bottom shadow strip
    draw.rectangle([BODY_L, BODY_BOT - 5 + bob, BODY_R, BODY_BOT + bob], fill=CLAWD_SHAD)
    # Legs
    for lx in LEG_XS:
        draw.rectangle([lx, LEG_Y0, lx + LEG_W, LEG_Y1], fill=CLAWD)
        draw.rectangle([lx, LEG_Y1 - 4, lx + LEG_W, LEG_Y1], fill=CLAWD_SHAD)
    # Eyes
    if closed:
        mid = EYE_Y + EYE_H2 // 2 + bob
        draw.rectangle([EYE_LX, mid - 1, EYE_LX + EYE_W2, mid + 1], fill=EYE)
        draw.rectangle([EYE_RX, mid - 1, EYE_RX + EYE_W2, mid + 1], fill=EYE)
    else:
        draw.rectangle([EYE_LX, EYE_Y + bob, EYE_LX + EYE_W2, EYE_Y + EYE_H2 + bob], fill=EYE)
        draw.rectangle([EYE_RX, EYE_Y + bob, EYE_RX + EYE_W2, EYE_Y + EYE_H2 + bob], fill=EYE)
        # Shine
        draw.rectangle([EYE_LX+1, EYE_Y+1+bob, EYE_LX+3, EYE_Y+3+bob], fill=(255,255,255))
        draw.rectangle([EYE_RX+1, EYE_Y+1+bob, EYE_RX+3, EYE_Y+3+bob], fill=(255,255,255))

def draw_rod(draw, bobber_off=0):
    draw.line([(ROD_BX, ROD_BY), (ROD_TX, ROD_TY)], fill=ROD_C, width=3)
    by = BOB_Y0 + bobber_off
    draw.line([(ROD_TX, ROD_TY), (BOB_X, by)], fill=LINE_C, width=1)
    draw.ellipse([BOB_X-5, by-5, BOB_X+5, by+5], fill=BOBBER_R)
    draw.ellipse([BOB_X-5, by-5, BOB_X+5, by],   fill=BOBBER_W)

def draw_pixel_z(draw, x, y, sz, alpha):
    """Pixel-art Z: top bar, diagonal, bottom bar."""
    c = (*ZZZ_C, alpha)
    t = max(1, sz // 5)
    draw.rectangle([x, y,           x + sz, y + t],          fill=c)  # top
    draw.rectangle([x, y + sz - t,  x + sz, y + sz],          fill=c)  # bottom
    steps = sz
    for s in range(steps + 1):
        px = x + sz - int(s * sz / steps)
        py = y + int(s * sz / steps)
        draw.rectangle([px, py, min(px+t, x+sz), min(py+t, y+sz)], fill=c)

def draw_zzz(img, f, total):
    overlay = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(overlay)
    # 3 Z's staggered in time
    sizes = [8, 12, 17]
    for z_idx, sz in enumerate(sizes):
        phase = (f + z_idx * (total // 3)) % total
        progress = phase / total              # 0 → 1
        if progress > 0.75:
            continue
        alpha = int(255 * (1 - progress / 0.75))
        zx = BODY_R + 6 + z_idx * 14
        zy = BODY_TOP - 8 - int(progress * 50)
        draw_pixel_z(d, zx, zy, sz, alpha)
    return Image.alpha_composite(img, overlay)


frames   = []
durations = []

# ─── DAY: Claw'd fishing ───────────────────────────────────────────────────
for f in range(DAY_FRAMES):
    t = f / DAY_FRAMES

    img  = Image.new('RGBA', (W, H))
    draw = ImageDraw.Draw(img)

    draw_sky_gradient(draw, SKY_DAY, lerp_rgb(SKY_DAY, (255, 220, 150), 0.25))

    # Clouds
    for cx2, cy2 in [(70, 28), (230, 18), (370, 35)]:
        for dx in [-15, 0, 15]:
            draw.ellipse([cx2+dx-22, cy2-10, cx2+dx+22, cy2+12], fill=(255,255,255,210))

    # Sun
    sx, sy = W - 75, 42
    for gr, ga in [(32,30),(24,55),(15,255)]:
        draw.ellipse([sx-gr, sy-gr, sx+gr, sy+gr], fill=(255,230,55,ga))

    draw_ocean(draw, OCEAN_DAY)

    # Waves
    for wx in range(-20, W + 20, 22):
        wy = HORIZON_Y + 5 + int(3 * math.sin((wx + f*3) * 0.09))
        draw.arc([wx-10, wy-3, wx+10, wy+4], 0, 180, fill=(255,255,255,150), width=2)

    draw_sand(draw, SAND_DAY)

    # Claw'd shadow
    draw.ellipse([BODY_L+6, SAND_Y+1, BODY_R-6, SAND_Y+7], fill=(170,140,90,70))

    # Bobber bob + fish splash
    bob_off = int(4 * math.sin(t * 2 * math.pi * 4))
    draw_rod(draw, bob_off)

    if 0.38 < t < 0.44:   # fish splash moment
        bx, by = BOB_X, BOB_Y0 + bob_off
        for r2 in [10, 7, 4]:
            draw.arc([bx-r2, by-r2//2, bx+r2, by+r2//2], 0, 180,
                     fill=(255,255,255,100), width=1)

    draw_clawd(draw, closed=False, bob=0)

    frames.append(img.convert('RGB'))
    durations.append(DUR_DAY)

# ─── NIGHT: Claw'd sleeping ────────────────────────────────────────────────
for f in range(NIGHT_FRAMES):
    t = f / NIGHT_FRAMES

    img  = Image.new('RGBA', (W, H))
    draw = ImageDraw.Draw(img)

    draw_sky_gradient(draw, SKY_NIGHT, lerp_rgb(SKY_NIGHT, (30, 45, 100), 0.5))

    # Stars
    for sx2, sy2 in STARS:
        twinkle = 0.65 + 0.35 * math.sin(f * 0.12 + sx2 * 0.25)
        r2 = 1 if sx2 % 3 != 0 else 2
        draw.ellipse([sx2-r2, sy2-r2, sx2+r2, sy2+r2], fill=(255,255,215,int(220*twinkle)))

    # Crescent moon
    mx2, my2 = W - 72, 38
    draw.ellipse([mx2-22, my2-22, mx2+22, my2+22], fill=(245,245,200))
    draw.ellipse([mx2- 9, my2-26, mx2+18, my2+14], fill=SKY_NIGHT)

    draw_ocean(draw, OCEAN_NIGHT)

    # Moon reflection
    for ry in range(HORIZON_Y + 3, SAND_Y, 3):
        a3 = int(55 * (1 - (ry - HORIZON_Y) / (SAND_Y - HORIZON_Y)))
        draw.line([(mx2-7, ry), (mx2+7, ry)], fill=(245,245,200,a3))

    draw_sand(draw, SAND_NIGHT)

    # Claw'd shadow
    draw.ellipse([BODY_L+6, SAND_Y+1, BODY_R-6, SAND_Y+7], fill=(100,82,58,55))

    # Breathing bob
    bob = int(1.5 * math.sin(t * 2 * math.pi * 1.5))
    draw_clawd(draw, closed=True, bob=bob)

    # Zzz overlay
    img = draw_zzz(img, f, NIGHT_FRAMES)

    frames.append(img.convert('RGB'))
    durations.append(DUR_NIGHT)

# ─── Save ──────────────────────────────────────────────────────────────────
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'mascot-beach.gif')
frames[0].save(
    out,
    save_all=True,
    append_images=frames[1:],
    loop=0,
    duration=durations,
    optimize=True,
)
total_s = (DAY_FRAMES * DUR_DAY + NIGHT_FRAMES * DUR_NIGHT) / 1000
print(f"✓ {DAY_FRAMES} day frames ({DAY_FRAMES*DUR_DAY/1000:.1f}s) + "
      f"{NIGHT_FRAMES} night frames ({NIGHT_FRAMES*DUR_NIGHT/1000:.1f}s) "
      f"= {total_s:.1f}s cycle")
print(f"  → {out}")
