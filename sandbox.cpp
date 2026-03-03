#include "core_init.h"

#include "color.h"
#include "surface.h"
#include "surface_renderer.h"
#include "tga_files.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

// TODO: Task for this experiment is to have a way to turn a utf-8 stream of unicode characters and turn those into
//       a bitmap that can be rasterized into a surface.

i32 main() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    [[maybe_unused]] const char* fontPath = ASSETS_DIRECTORY "/test_assets/fonts/NotoSans-Regular.ttf";
    [[maybe_unused]] const char* output = OUT_DIRECTORY "/output.tga";

    FT_MemoryRec_ mem = {};
    mem.user = reinterpret_cast<void*>(&core::getAllocator(RA_STD_ALLOCATOR_ID));
    mem.alloc = [](FT_Memory m, long size) -> void* {
        auto& actx = *reinterpret_cast<core::AllocatorContext*>(m->user);
        return actx.alloc(addr_size(size), sizeof(u8));
    };
    mem.realloc = [](FT_Memory m, long currSize, long newSize, void* ptr) -> void* {
        auto& actx = *reinterpret_cast<core::AllocatorContext*>(m->user);
        return actx.reallocate(ptr, addr_size(newSize), sizeof(u8), addr_size(currSize), sizeof(u8));
    };
    mem.free = [](FT_Memory m, void* ptr) {
        auto& actx = *reinterpret_cast<core::AllocatorContext*>(m->user);
        actx.free(ptr, 0, 0);
    };

    // Create lib
    FT_Library lib = nullptr;
    if (auto errCode = FT_New_Library(&mem, &lib); errCode != 0) {
        const char* errStr = FT_Error_String(errCode);
        logErr("FT_Init_FreeType failed; reason: {}", errStr);
        return errCode;
    }
    defer { FT_Done_Library(lib); };
    FT_Add_Default_Modules(lib);

    // Print version
    FT_Int major = 0; FT_Int minor = 0; FT_Int patch = 0;
    FT_Library_Version(lib, &major, &minor, &patch);
    logInfo("FreeType initialized: {}.{}.{}", major, minor, patch);

    // Create font face from ttf file
    FT_Face face;
    if (auto errCode = FT_New_Face(lib, fontPath, 0, &face); errCode != 0) {
        const char* errStr = FT_Error_String(errCode);
        logErr("FT_New_Face failed; reason: {}", errStr);
        return errCode;
    }
    defer { FT_Done_Face(face); };

    logInfo("Loaded '{} {}'", face->family_name, face->style_name);
    logInfo("Glyph Count = {}", face->num_glyphs);

    constexpr i32 FONT_PIXEL_HEIGHT = 50;
    if (auto errCode = FT_Set_Pixel_Sizes(face, 0, FONT_PIXEL_HEIGHT); errCode != 0) {
        const char* errStr = FT_Error_String(errCode);
        logErr("FT_Set_Pixel_Sizes failed; reason: {}", errStr);
        return errCode;
    }

    FT_ULong charCode = 235; // ë
    if (auto errCode = FT_Load_Char(face, charCode, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT); errCode != 0) {
        const char* errStr = FT_Error_String(errCode);
        logErr("FT_Load_Char failed; reason: {}", errStr);
        return errCode;
    }

    FT_GlyphSlot gslot = face->glyph;
    i32 w = i32(gslot->bitmap.width);
    i32 h = i32(gslot->bitmap.rows);
    i32 bearingX = gslot->bitmap_left;
    i32 bearingY = gslot->bitmap_top;
    i32 advancePx = i32(gslot->advance.x) >> 6;
    u8* alpha = gslot->bitmap.buffer;

    logInfo("w={}, h={}, bearingX={}, bearingY={} advancePx={}", w, h, bearingX, bearingY, advancePx);

    //==================================================================================================================
    // Create Surface
    //==================================================================================================================

    constexpr PixelFormat f = PixelFormat::GRAY8;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);

    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::TopLeft;
    s.pixelFormat = f;
    s.width = w;
    s.height = h;
    s.pitch = s.width * bpp;
    s.data = alpha;
    defer { s.free(); };

    //==================================================================================================================
    // Render
    //==================================================================================================================

    // fillRect(s, 0, 0, BLACK, s.width, s.height);

    //==================================================================================================================
    // Write to output tga file
    //==================================================================================================================

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = output,
        .imageType = 3,
        .fileType = TGA::FileType::New,
    };
    Expect(TGA::createFileFromSurface(params));
    logInfo("Created output file in \"{}\"", params.path);

    logInfo("Done");
    return 0;
}
