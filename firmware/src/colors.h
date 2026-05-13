#pragma once
#include <cstdint>

namespace Colors {
    constexpr uint16_t CRAIL       = 0xC2E7; // #C15F3C
    constexpr uint16_t CRAIL_LIGHT = 0xEC2B; // #E8845C
    constexpr uint16_t CLOUDY      = 0xB574; // #B1ADA1
    constexpr uint16_t PAMPAS      = 0xF79D; // #F4F3EE
    constexpr uint16_t DARK_BG     = 0x0840; // #0D0A07
    constexpr uint16_t DARK_CARD   = 0x18A2; // #1A1410
    constexpr uint16_t DARK_BORDER = 0x3943; // #3A2A1E
    constexpr uint16_t WHITE       = 0xFFDF; // #F8FAFC
    constexpr uint16_t GRAY        = 0x7BEF; // #808080
    constexpr uint16_t TRANSPARENT = 0xF81F; // magenta, sprite key

    inline uint16_t lerp(uint16_t c1, uint16_t c2, float t) {
        int r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
        int r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
        int r = r1 + (int)((r2 - r1) * t);
        int g = g1 + (int)((g2 - g1) * t);
        int b = b1 + (int)((b2 - b1) * t);
        return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
    }
}
