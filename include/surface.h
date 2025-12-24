#include "core_init.h"
#include "error.h"

namespace TGA { struct TGAFile; }

enum struct PixelFormat {
    Unknown,

    SENTINEL
};

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
