#pragma once

#include "core_init.h"
#include "error.h"

namespace TGA { struct TGAFile; }

enum struct PixelFormat {
    Unknown,

    BGRA8888,
    BGRX8888,
    BGRA5551,
    BGR555,
    BGR888,

    SENTINEL
};

constexpr i32 pixelFormatBytesPerPixel(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return 4;
        case PixelFormat::BGRX8888: return 4;
        case PixelFormat::BGRA5551: return 2;
        case PixelFormat::BGR555:   return 2;
        case PixelFormat::BGR888:   return 3;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return -1;
    }
}

constexpr i32 pixelFormatAlphaBits(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return 8;
        case PixelFormat::BGRX8888: return 0;
        case PixelFormat::BGRA5551: return 1;
        case PixelFormat::BGR555:   return 0;
        case PixelFormat::BGR888:   return 0;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return false;
    }
}

constexpr const char* pixelFormatToCstr(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return "BGRA_8888";
        case PixelFormat::BGRX8888: return "BGRX_8888";
        case PixelFormat::BGRA5551: return "BGRA_5551";
        case PixelFormat::BGR555:   return "BGR_555";
        case PixelFormat::BGR888:   return "BGR_888";

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default: return "unknown";
    }
}

enum struct Origin {
    Undefined,

    BottomLeft,
    BottomRight,
    TopLeft,
    TopRight,
    Center,

    SENTINEL
};

struct Surface {
    core::AllocatorContext* actx;

    Surface()
        : actx(nullptr)
        , origin(Origin::Undefined)
        , pixelFormat(PixelFormat::Unknown)
        , width(0)
        , height(0)
        , pitch(0)
        , data(nullptr) {}

    Origin origin;
    PixelFormat pixelFormat;
    i32 width;
    i32 height;
    i32 pitch;
    u8* data;

    constexpr i32 size() const { return height * pitch; }
    constexpr i32 bpp() const { return pixelFormatBytesPerPixel(pixelFormat); }
    constexpr bool isOwner() const { return actx != nullptr; }
    void free();
};

core::expected<Surface, Error> createSurfaceFromTgaFile(const TGA::TGAFile& tgaFile,
                                                        core::AllocatorContext& actx = DEF_ALLOC);
