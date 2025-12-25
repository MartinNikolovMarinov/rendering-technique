#include "core_init.h"
#include "error.h"

namespace TGA { struct TGAFile; }

enum struct PixelFormat {
    Unknown,

    ABGR8888,
    BGR888,

    SENTINEL
};

constexpr i32 pixelFormatBytesPerPixel(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::ABGR8888: return 4;
        case PixelFormat::BGR888:   return 3;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return -1;
    }
}

constexpr bool pixelFormatHasAlpha(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::ABGR8888: return true;
        case PixelFormat::BGR888:   return false;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return false;
    }
}

struct Surface {
    core::AllocatorContext* actx;

    PixelFormat pixelFormat;
    i32 width;
    i32 height;
    i32 pitch;
    u8* data;

    constexpr i32 size() { return height * pitch; }
    void free();
};

core::expected<Surface, Error> createSurfaceFromTgaFile(const TGA::TGAFile& tgaFile,
                                                        core::AllocatorContext& actx = DEF_ALLOC);
