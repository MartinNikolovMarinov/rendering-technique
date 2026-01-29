#pragma once

#include "core_init.h"

struct Model3D;
struct Surface;
struct Color;
struct Face3i;

using Vertex4f = core::vec4f;

//======================================================================================================================
// Direct Rendering to a Surface Section
//======================================================================================================================

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
    const Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
);
void fillTriangle(
    Surface& surface,
    const Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC
);

//======================================================================================================================
// Statefull Rendering Section
//======================================================================================================================

struct Renderer;
using RendererHandle = Renderer*;

RendererHandle rendererInit(core::AllocatorContext& actx);
void rendererDestory(RendererHandle r);

void rendererSetFrameBuffer(RendererHandle r, i32 width, i32 height);
void rendererSetWireframe(RendererHandle r, bool wireframe);
void rendererSetOutput(RendererHandle r, Surface& output);

void rendererBeginFrame(RendererHandle r);

void rendererClear(RendererHandle r, const Color& c);
void rendererSetVertexBuffer(RendererHandle r, core::Memory<Vertex4f> vertices);
void rendererSetIndexBuffer(RendererHandle r, core::Memory<Face3i> indices);
void rendererCalculateDepthBuffer(RendererHandle r, Surface& depthBuffer);

void rendererEndFrame(RendererHandle r);
