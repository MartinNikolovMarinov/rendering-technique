#include "surface_renderer.h"

#include "color.h"
#include "face.h"
#include "model.h"
#include "face.h"
#include "surface.h"

namespace {

using SetPixelFn = void (*)(u8* data, i32 idx, Color color);
using GetPixelFn = Color (*)(u8* data, i32 idx);

constexpr inline void setPixelTopLeft_BGRA8888(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGR888(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGRA5551(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGR555(u8* data, i32 idx, Color color);

constexpr inline SetPixelFn pickSetPixelFunction(PixelFormat pixelFormat);

constexpr inline Color getPixelTopLeft_BGRA8888(u8* data, i32 idx);
constexpr inline Color getPixelTopLeft_BGRX8888(u8* data, i32 idx);
constexpr inline Color getPixelTopLeft_BGR888(u8* data, i32 idx);
constexpr inline Color getPixelTopLeft_BGRA5551(u8* data, i32 idx);
constexpr inline Color getPixelTopLeft_BGR555(u8* data, i32 idx);

constexpr inline GetPixelFn pickGetPixelFunction(PixelFormat pixelFormat);

} // namespace

void fillPixel(Surface& surface, i32 x, i32 y, Color color) {
    i32 idx = y * surface.pitch + x * surface.bpp();

    Assert(surface.data != nullptr, "surface data is null");
    Assert(y >= 0 && y < surface.height, "y out of bounds");
    Assert(x >= 0 && x < surface.width, "x out of bounds");
    Assert(idx + surface.bpp() <= surface.size(), "pixel write past end of surface");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
    setPixelFn(surface.data, idx, color);
}

// FIXME: Remove this
Color getPixelAt(const Surface& surface, i32 x, i32 y) {
    i32 idx = y * surface.pitch + x * surface.bpp();

    Assert(surface.data != nullptr, "surface data is null");
    Assert(y >= 0 && y < surface.height, "y out of bounds");
    Assert(x >= 0 && x < surface.width, "x out of bounds");
    Assert(idx + surface.bpp() <= surface.size(), "pixel write past end of surface");

    GetPixelFn getPixelAt = pickGetPixelFunction(surface.pixelFormat);
    auto ret = getPixelAt(surface.data, idx);
    return ret;
}

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color) {
    Assert(surface.data != nullptr, "surface data is null");
    Assert(ax >= 0 && ay >= 0 && bx >= 0 && by >= 0, "line start/end out of bounds (negative)");
    Assert(ax < surface.width && bx < surface.width, "line x out of bounds");
    Assert(ay < surface.height && by < surface.height, "line y out of bounds");
    Assert(surface.bpp() > 0, "invalid bytes-per-pixel");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);

    // Bresenham line drawing algorithm using integer calculations.

    bool transpose = core::absGeneric(ax - bx) < core::absGeneric(ay - by);
    if (transpose) {
        core::swap(ax, ay);
        core::swap(bx, by);
    }

    bool flipLeftToRight = ax > bx;
    if (flipLeftToRight) {
        core::swap(ax, bx);
        core::swap(ay, by);
    }

    i32 y = ay;
    i32 ierror = 0;
    for (i32 x = ax; x <= bx; x++) {
        if (transpose) {
            i32 idx = x * surface.pitch + y * surface.bpp();
            setPixelFn(surface.data, idx, color);
        }
        else {
            i32 idx = y * surface.pitch + x * surface.bpp();
            setPixelFn(surface.data, idx, color);
        }

        ierror += i32(2 * core::absGeneric(by - ay));
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx-ax);
        }
    }
}

void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height) {
    Assert(surface.data != nullptr, "surface data is null");
    Assert(width > 0 && height > 0, "rect has non-positive size");
    Assert(x >= 0 && y >= 0, "rect origin out of bounds");
    Assert(y + height <= surface.height, "rect extends past surface height");
    Assert(x + width  <= surface.width,  "rect extends past surface width");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
    for (i32 row = y; row < y + height; row++) {
        for (i32 col = x; col < x + width; col++) {
            i32 idx = row * surface.pitch + col * surface.bpp();
            setPixelFn(surface.data, idx, color);
        }
    }
}

void strokeRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height) {
    i32 endX = x + width - 1;
    i32 endY = y + height - 1;
    fillLine(surface, x, y, endX, y, color);
    fillLine(surface, endX, y, endX, endY, color);
    fillLine(surface, endX, endY, x, endY, color);
    fillLine(surface, x, endY, x, y, color);
}

namespace {

void fillTriangleBarycentric(
    Surface& surface,
    const Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 holeInsetRatio
) {
    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a.xy(), b.xy(), c.xy());

    f32 totalArea = core::calcTriangleAreaF32(a.xy(), b.xy(), c.xy());
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.x(), b.y(), c.x(), c.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.x(), c.y(), a.x(), a.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.x(), a.y(), b.x(), b.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            u8 z = u8(alpha * f32(a.z()) + beta * f32(b.z()) + gamma * f32(c.z()));

            // TODO: Remove this get pixel function it's trash..
            u8 depth = getPixelAt(depthBuffer, x, y).g();
            if (z < depth) {
                continue;
            }

            if (holeInsetRatio > 0.0f) {
                f32 t = holeInsetRatio / 3.0f;
                if (alpha >= t && beta >= t && gamma >= t) {
                    continue;
                }
            }

            Color blendedColor = {
                .rgba {
                    .r = u8(alpha * f32(colorA.r()) + beta * f32(colorB.r()) + gamma * f32(colorC.r())),
                    .g = u8(alpha * f32(colorA.g()) + beta * f32(colorB.g()) + gamma * f32(colorC.g())),
                    .b = u8(alpha * f32(colorA.b()) + beta * f32(colorB.b()) + gamma * f32(colorC.b())),
                    .a = u8(alpha * f32(colorA.a()) + beta * f32(colorB.a()) + gamma * f32(colorC.a()))
                }
            };

            fillPixel(surface, x, y, blendedColor);
        }
    }
}

void fillDepthBuffer(
    Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c
) {
    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a.xy(), b.xy(), c.xy());

    f32 totalArea = core::calcTriangleAreaF32(a.xy(), b.xy(), c.xy());
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.x(), b.y(), c.x(), c.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.x(), c.y(), a.x(), a.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.x(), a.y(), b.x(), b.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            u8 z = u8(alpha * f32(a.z()) + beta * f32(b.z()) + gamma * f32(c.z()));

            u8 depth = getPixelAt(depthBuffer, x, y).g();
            if (z <= depth) {
                continue;
            }

            Color grayColor = { .rgba { z, z, z, 255 } };
            fillPixel(depthBuffer, x, y, grayColor);
        }
    }
}

} // namespace

void strokeTriangleFast(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& color
) {
    fillLine(surface, a.x(), a.y(), b.x(), b.y(), color);
    fillLine(surface, b.x(), b.y(), c.x(), c.y(), color);
    fillLine(surface, c.x(), c.y(), a.x(), a.y(), color);
}

void strokeTriangleInset(
    Surface& surface,
    const Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
) {
    f32 clampedRatio = core::core_max(0.0f, core::core_min(boarderRatio, 1.0f));
    fillTriangleBarycentric(surface, depthBuffer, a, b, c, colorA, colorB, colorC, clampedRatio);
}

void fillTriangle(
    Surface& surface,
    const Surface& depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC
) {
    fillTriangleBarycentric(surface, depthBuffer, a, b, c, colorA, colorB, colorC, 0.0f);
}

//======================================================================================================================
// Statefull Rendering Section
//======================================================================================================================

namespace {

constexpr inline core::vec3i orthogonalProjection(core::vec4f normVec, i32 width, i32 height, i32 depth) {
    i32 x = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
    i32 y = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
    i32 z = i32((normVec.z() + 1.0f) * (f32(depth - 1)/2.0f));
    return core::v(x, y, z);
}

}

struct RenderPassState {
    Surface* depthBuffer;
    core::Memory<Vertex4f> vertices;
    core::Memory<Face3i> faces;
};

struct Renderer {
    core::AllocatorContext* actx;
    i32 frameBufferWidth;
    i32 frameBufferHeight;
    bool wireframe;
    Surface* output;

    RenderPassState renderPass;
};

RendererHandle rendererInit(core::AllocatorContext& actx) {
    Renderer* ret = reinterpret_cast<Renderer*>(actx.alloc(1, sizeof(Renderer)));
    *ret = {};
    ret->actx = &actx;
    return ret;
}

void rendererDestory(RendererHandle r) {
    if (r && r->actx) {

        // Finalize free the renderer itself and zero-out all fields:
        r->actx->free(r, 1, sizeof(Renderer));
        r = {};
    }
}

void rendererSetFrameBuffer(RendererHandle r, i32 width, i32 height) {
    r->frameBufferWidth = width;
    r->frameBufferHeight = height;
}

void rendererSetWireframe(RendererHandle r, bool wireframe) {
    r->wireframe = wireframe;
}

void rendererSetOutput(RendererHandle r, Surface& output) {
    r->output = &output;
}

void rendererBeginFrame(RendererHandle) {
}

void rendererClear(RendererHandle r, const Color& c) {
    fillRect(*r->output, 0, 0, c, r->frameBufferWidth, r->frameBufferHeight);
}

void rendererSetVertexBuffer(RendererHandle r, core::Memory<Vertex4f> vertices) {
    r->renderPass.vertices = vertices;
}

void rendererSetIndexBuffer(RendererHandle r, core::Memory<Face3i> indices) {
    r->renderPass.faces = indices;
}

void rendererCalculateDepthBuffer(RendererHandle r, Surface& depthBuffer) {
    r->renderPass.depthBuffer = &depthBuffer;

    bool wireframeMode = r->wireframe;

    // Calculate depth buffer:
    if (!wireframeMode) {
        auto& vertices = r->renderPass.vertices;
        auto& faces = r->renderPass.faces;

        i32 width = r->frameBufferWidth;
        i32 height = r->frameBufferHeight;

        for (addr_size i = 0; i < faces.len(); i++) {
            auto& f = faces[i];

            core::vec4f& v1 = vertices[f[0]];
            core::vec4f& v2 = vertices[f[1]];
            core::vec4f& v3 = vertices[f[2]];

            // TODO: I need to think about this hardcoded 256 value..
            core::vec3i a = orthogonalProjection(v1, width, height, 256);
            core::vec3i b = orthogonalProjection(v2, width, height, 256);
            core::vec3i c = orthogonalProjection(v3, width, height, 256);

            fillDepthBuffer(depthBuffer, a, b, c);
        }
    }
}

void rendererEndFrame(RendererHandle r) {
    auto& surface = *r->output;
    bool wireframe = r->wireframe;
    auto& vertices = r->renderPass.vertices;
    auto& faces = r->renderPass.faces;
    auto& depthBuffer = *r->renderPass.depthBuffer;

    Assert(surface.width>= r->frameBufferWidth);
    Assert(surface.height >= r->frameBufferHeight);
    i32 width = surface.width;
    i32 height = surface.height;

    for (addr_size i = 0; i < faces.len(); i++) {
        auto& f = faces[i];

        core::vec4f& v1 = vertices[f[0]];
        core::vec4f& v2 = vertices[f[1]];
        core::vec4f& v3 = vertices[f[2]];

        // TODO: I need to think about this hardcoded 256 value..
        core::vec3i a = orthogonalProjection(v1, width, height, 256);
        core::vec3i b = orthogonalProjection(v2, width, height, 256);
        core::vec3i c = orthogonalProjection(v3, width, height, 256);

        if (wireframe) {
            strokeTriangleFast(surface, a.xy(), b.xy(), c.xy(), RED);
            fillPixel(surface, a.x(), a.y(), WHITE);
            fillPixel(surface, b.x(), b.y(), WHITE);
            fillPixel(surface, c.x(), c.y(), WHITE);
        }
        else {
            Color color1 = randomColor();
            Color color2 = randomColor();
            Color color3 = randomColor();
            fillTriangle(surface, depthBuffer, a, b, c, color1, color2, color3);
        }
    }
}

namespace {

constexpr inline void setPixelTopLeft_BGRA8888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
    data[idx + 3] = color.a();
}

constexpr inline void setPixelTopLeft_BGRX8888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
    data[idx + 3] = 0;
}

constexpr inline void setPixelTopLeft_BGR888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
}

constexpr inline void setPixelTopLeft_BGRA5551(u8* data, i32 idx, Color color) {
    // Packed as: bits 0-4 blue, 5-9 green, 10-14 red, 15 alpha.
    u16 b = u16(color.b() >> 3);
    u16 g = u16(color.g() >> 3);
    u16 r = u16(color.r() >> 3);
    u16 a = u16(color.a() >> 7);
    u16 packed = u16(b | (g << 5) | (r << 10) | (a << 15));
    data[idx + 0] = u8(packed & 0xFF);
    data[idx + 1] = u8(packed >> 8);
}

constexpr inline void setPixelTopLeft_BGR555(u8* data, i32 idx, Color color) {
    // Packed as: bits 0-4 blue, 5-9 green, 10-14 red, bit 15 cleared.
    u16 b = u16(color.b() >> 3);
    u16 g = u16(color.g() >> 3);
    u16 r = u16(color.r() >> 3);
    u16 packed = u16(b | (g << 5) | (r << 10));
    data[idx + 0] = u8(packed & 0xFF);
    data[idx + 1] = u8(packed >> 8);
}

constexpr inline SetPixelFn pickSetPixelFunction(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return setPixelTopLeft_BGRA8888;
        case PixelFormat::BGRX8888: return setPixelTopLeft_BGRX8888;
        case PixelFormat::BGR888:   return setPixelTopLeft_BGR888;
        case PixelFormat::BGRA5551: return setPixelTopLeft_BGRA5551;
        case PixelFormat::BGR555:   return setPixelTopLeft_BGR555;

        case PixelFormat::GRAY8:    Assert(false, "TODO:"); return nullptr;
        case PixelFormat::GRAYA88:  Assert(false, "TODO:"); return nullptr;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return nullptr;
    }
}

constexpr inline Color getPixelTopLeft_BGRA8888(u8* data, i32 idx) {
    Color ret = {
        .rgba {
            .r = data[idx + 2],
            .g = data[idx + 1],
            .b = data[idx + 0],
            .a = data[idx + 3],
        }
    };
    return ret;
}

constexpr inline Color getPixelTopLeft_BGRX8888(u8* data, i32 idx) {
    Color ret = {
        .rgba {
            .r = data[idx + 2],
            .g = data[idx + 1],
            .b = data[idx + 0],
            .a = 0,
        }
    };
    return ret;
}

constexpr inline Color getPixelTopLeft_BGR888(u8* data, i32 idx) {
    Color ret = {
        .rgba {
            .r = data[idx + 2],
            .g = data[idx + 1],
            .b = data[idx + 0],
            .a = 0,
        }
    };
    return ret;
}


constexpr inline GetPixelFn pickGetPixelFunction(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return getPixelTopLeft_BGRA8888;
        case PixelFormat::BGRX8888: return getPixelTopLeft_BGRX8888;
        case PixelFormat::BGR888:   return getPixelTopLeft_BGR888;

        case PixelFormat::GRAY8:      Assert(false, "TODO:"); return nullptr;
        case PixelFormat::GRAYA88:    Assert(false, "TODO:"); return nullptr;

        case PixelFormat::BGRA5551: [[fallthrough]];
        case PixelFormat::BGR555:   [[fallthrough]];
        case PixelFormat::Unknown:  [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return nullptr;
    }
}


} // namespace
