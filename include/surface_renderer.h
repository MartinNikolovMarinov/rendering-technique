#pragma once

#include "surface.h"

struct Model3D;

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
constexpr Color WHITE = Color { .rgba = { 255, 255, 255, 255 } };
constexpr Color BLACK = Color { .rgba = { 0, 0, 0, 255 } };
constexpr Color GRAY = Color { .rgba = { 128, 128, 128, 255 } };

void fillPixel(Surface& surface, i32 x, i32 y, Color color);

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color);

void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
void strokeRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);

void strokeTriangleFast(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color color
);
void strokeTriangleInset(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color colorA, Color colorB, Color colorC,
    f32 boarderRatio
);
void fillTriangle(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color colorA, Color colorB, Color colorC
);

// TODO: pass mvp matrix ?
void renderModel(Surface& surface, const Model3D& model, bool wireframe = false);
