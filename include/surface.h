#pragma once

#include "core_init.h"

enum struct PixelFormat {
    Unknown,

    BGRA8888,
    BGRX8888,
    BGRA5551,
    BGR555,
    BGR888,
    GRAY8,
    GRAYA88,

    SENTINEL
};

constexpr i32 pixelFormatBytesPerPixel(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return 4;
        case PixelFormat::BGRX8888: return 4;
        case PixelFormat::BGRA5551: return 2;
        case PixelFormat::BGR555:   return 2;
        case PixelFormat::BGR888:   return 3;
        case PixelFormat::GRAY8:    return 1;
        case PixelFormat::GRAYA88:  return 2;

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
        case PixelFormat::GRAY8:    return 0;
        case PixelFormat::GRAYA88:  return 8;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return -1;
    }
}

constexpr const char* pixelFormatToCstr(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return "BGRA_8888";
        case PixelFormat::BGRX8888: return "BGRX_8888";
        case PixelFormat::BGRA5551: return "BGRA_5551";
        case PixelFormat::BGR555:   return "BGR_555";
        case PixelFormat::BGR888:   return "BGR_888";
        case PixelFormat::GRAY8:    return "GRAY_8";
        case PixelFormat::GRAYA88:  return "GRAYA_88";

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

constexpr const char* originToCstr(Origin o) {
    switch (o) {
        case Origin::BottomLeft:  return "BottomLeft";
        case Origin::BottomRight: return "BottomRight";
        case Origin::TopLeft:     return "TopLeft";
        case Origin::TopRight:    return "TopRight";
        case Origin::Center:      return "Center";

        case Origin::Undefined: [[fallthrough]];
        case Origin::SENTINEL: [[fallthrough]];
        default: return "undefined";
    }
}

struct Surface {
    core::AllocatorContext* actx = nullptr;
    Origin origin = Origin::Undefined;
    PixelFormat pixelFormat = PixelFormat::Unknown;
    i32 width = 0;
    i32 height = 0;
    i32 pitch = 0;
    u8* data = nullptr; // FIXME: [BUG] Accessing data directly is a problem if the data is packed and for BGR555 it will be.

    constexpr i32 size() const { return height * pitch; }
    constexpr i32 bpp() const { return pixelFormatBytesPerPixel(pixelFormat); }
    constexpr bool isOwner() const { return actx != nullptr; }
    void free();
};
