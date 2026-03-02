#pragma once

#include "core_init.h"

struct Model3D;
struct Surface;
struct DepthBuffer;
struct Color;
struct Face3i;

//======================================================================================================================
// Direct Rendering to a Surface Section
//======================================================================================================================

void fillPixel(Surface& surface, i32 x, i32 y, Color color);
void fillPixelLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color);

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color);
void fillLineLocal(Surface& surface, const ViewPort& viewport, i32 arelX, i32 arelY, i32 brelX, i32 brelY, Color color);

void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
void strokeRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height);
void fillRectLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color, i32 width, i32 height);
void strokeRectLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color, i32 width, i32 height);
void strokeViewport(Surface& surface, const ViewPort& viewport, Color color);

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
void strokeTriangleFastLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& color
);
void strokeTriangleInsetLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
);
void fillTriangleLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& colorA, const Color& colorB, const Color& colorC
);

//======================================================================================================================
// Statefull Rendering Section
//======================================================================================================================

struct Renderer;
using RendererHandle = Renderer*;

RendererHandle rendererInit(core::AllocatorContext& actx);
void rendererDestory(RendererHandle r);

void rendererSetViewport(RendererHandle r, ViewPort viewport);
void rendererSetWireframe(RendererHandle r, bool wireframe);
void rendererSetOutput(RendererHandle r, Surface& output);

void rendererBeginFrame(RendererHandle r);

void rendererClear(RendererHandle r, const Color& c);
void rendererSetVertexBuffer(RendererHandle r, core::Memory<Vertex4f> vertices);
void rendererSetIndexBuffer(RendererHandle r, core::Memory<Face3i> indices);
void rendererCalculateDepthBuffer(RendererHandle r, DepthBuffer& depthBuffer);

void rendererColorPass(RendererHandle r);
void rendererDepthColorPass(RendererHandle r);

void rendererEndFrame(RendererHandle r);
