#pragma once

#include "surface.h"

struct Color {
    struct RGBA { u8 r, g, b, a; };

    union {
        RGBA rgba;
        u8 colors[4];
    };

    constexpr u8 r() const { return rgba.r; }
    constexpr u8 g() const { return rgba.g; }
    constexpr u8 b() const { return rgba.b; }
    constexpr u8 a() const { return rgba.a; }
};

void setPixelTopLeft(Surface& surface, i32 x, i32 y, Color color);
void fillRectTopLeft(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
