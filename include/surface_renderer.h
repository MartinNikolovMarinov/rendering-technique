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

constexpr Color BLUE = Color { .rgba = { 0, 0, 255, 255 } };
constexpr Color RED = Color { .rgba = { 255, 0, 0, 255 } };
constexpr Color GREEN = Color { .rgba = { 0, 255, 0, 255 } };
constexpr Color YELLOW = Color { .rgba = { 255, 255, 0, 255 } };

void fillPixel(Surface& surface, i32 x, i32 y, Color color);
void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color);
