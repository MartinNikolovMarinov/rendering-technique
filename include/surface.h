#include "core_init.h"
#include "error.h"

namespace TGA { struct TGAFile; }

enum struct PixelFormat {
    Unknown,

    SENTINEL
};

struct Surface {
    PixelFormat pixelFormat;
    i32 width;
    i32 height;
    i32 pitch;
    u8* data;
};

core::expected<Surface, Error> createSurfaceFromTgaFile(TGA::TGAFile& tgaFile);
