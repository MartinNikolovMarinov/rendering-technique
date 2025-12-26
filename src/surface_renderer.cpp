#include "surface_renderer.h"
#include "surface.h"

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

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color) {
    Assert(surface.data != nullptr, "surface data is null");
    Assert(ax >= 0 && ay >= 0 && bx >= 0 && by >= 0, "line start/end out of bounds (negative)");
    Assert(ax < surface.width && bx < surface.width, "line x out of bounds");
    Assert(ay < surface.height && by < surface.height, "line y out of bounds");
    Assert(surface.bpp() > 0, "invalid bytes-per-pixel");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);

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

    for (i32 x = ax; x <= bx; x++) {
        f32 t = f32(x-ax) / f32(bx-ax);
        i32 y = i32(core::round(f32(ay) + f32(by - ay)*t));

        if (transpose) {
            i32 idx = x * surface.pitch + y * surface.bpp();
            setPixelFn(surface.data, idx, color);
        }
        else {
            i32 idx = y * surface.pitch + x * surface.bpp();
            setPixelFn(surface.data, idx, color);
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
