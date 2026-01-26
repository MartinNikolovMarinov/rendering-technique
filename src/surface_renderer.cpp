#include "surface_renderer.h"
#include "surface.h"
#include "model.h"

namespace {

using SetPixelFn = void (*)(u8* data, i32 idx, Color color);

constexpr inline void setPixelTopLeft_BGRA8888(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGR888(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGRA5551(u8* data, i32 idx, Color color);
constexpr inline void setPixelTopLeft_BGR555(u8* data, i32 idx, Color color);

constexpr inline SetPixelFn pickSetPixelFunction(PixelFormat pixelFormat);

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
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color colorA, Color colorB, Color colorC,
    f32 holeInsetRatio
) {
    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a, b, c);

    f32 totalArea = core::calcTriangleAreaF32(a, b, c);
    Assert(core::absGeneric(totalArea) >= 1, "Trying to draw triangle with area less than a pixel");

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

} // namespace

void strokeTriangleFast(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color color
) {
    fillLine(surface, a.x(), a.y(), b.x(), b.y(), color);
    fillLine(surface, b.x(), b.y(), c.x(), c.y(), color);
    fillLine(surface, c.x(), c.y(), a.x(), a.y(), color);
}

void strokeTriangleInset(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color colorA, Color colorB, Color colorC,
    f32 boarderRatio
) {
    f32 clampedRatio = core::core_max(0.0f, core::core_min(boarderRatio, 1.0f));
    fillTriangleBarycentric(surface, a, b, c, colorA, colorB, colorC, clampedRatio);
}

void fillTriangle(
    Surface& surface,
    core::vec2i a, core::vec2i b, core::vec2i c,
    Color colorA, Color colorB, Color colorC
) {
    fillTriangleBarycentric(surface, a, b, c, colorA, colorB, colorC, 0.0f);
}

void renderModel(Surface& surface, const Model3D& model, bool wireframe) {
    i32 width = surface.width;
    i32 height = surface.height;

    auto orthogonalProjection = [](core::vec4f normVec, i32 width, i32 height) -> core::vec2i {
        i32 ax = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
        i32 ay = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
        return core::v(ax, ay);
    };

    for (addr_size i = 0; i < model.faces.len(); i++) {
        auto& f = model.faces[i];

        core::vec4f& v1 = model.vertices[f[0]];
        core::vec4f& v2 = model.vertices[f[1]];
        core::vec4f& v3 = model.vertices[f[2]];

        core::vec2i a = orthogonalProjection(v1, width, height);
        core::vec2i b = orthogonalProjection(v2, width, height);
        core::vec2i c = orthogonalProjection(v3, width, height);

        f32 totalArea = core::calcTriangleAreaF32(a.x(), a.y(), b.x(), b.y(), c.x(), c.y());
        if (totalArea < 1) {
            // TODO: Do z-buffer test instead.
            // Naive backface culling;
            continue;
        }

        if (wireframe) {
            strokeTriangleFast(surface, a, b, c, RED);
            fillPixel(surface, a.x(), a.y(), BLUE);
            fillPixel(surface, b.x(), b.y(), BLUE);
            fillPixel(surface, c.x(), c.y(), BLUE);
        }
        else {
            Color color1 = randomColor();
            Color color2 = randomColor();
            Color color3 = randomColor();
            fillTriangle(surface, a, b, c, color1, color2, color3);
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

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return nullptr;
    }
}

} // namespace
