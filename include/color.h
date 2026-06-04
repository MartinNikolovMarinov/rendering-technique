#pragma once

#include "core_init.h" // IWYU pragma: keep

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
static_assert(std::is_standard_layout_v<Color>);

constexpr Color BLUE = Color { .rgba = { 0, 0, 255, 255 } };
constexpr Color RED = Color { .rgba = { 255, 0, 0, 255 } };
constexpr Color GREEN = Color { .rgba = { 0, 255, 0, 255 } };
constexpr Color YELLOW = Color { .rgba = { 255, 255, 0, 255 } };
constexpr Color WHITE = Color { .rgba = { 255, 255, 255, 255 } };
constexpr Color BLACK = Color { .rgba = { 0, 0, 0, 255 } };
constexpr Color GRAY = Color { .rgba = { 128, 128, 128, 255 } };

Color randomColor();
