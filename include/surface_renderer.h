#pragma once

#include "core_init.h"
#include "surface.h"
#include "color.h"

struct Model3D;

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
