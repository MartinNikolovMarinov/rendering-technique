#pragma once

#include "core_init.h"

struct Model3D;
struct Surface;
struct Color;

void fillPixel(Surface& surface, i32 x, i32 y, Color color);

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color);

void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
void strokeRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);

void strokeTriangleFast(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& color
);
void strokeTriangleInset(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
);
void fillTriangle(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& colorA, const Color& colorB, const Color& colorC
);

// TODO: pass mvp matrix ?
void renderModel(Surface& surface, const Model3D& model, bool wireframe = false);
