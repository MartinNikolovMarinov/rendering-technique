#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "color.h"

core::Profiler profiler_1;

enum ProfilePoints {
    PP_RESERVED,

    PP_DRAW_LINE,
};

void create5MillionLines(const char* path) {
    constexpr PixelFormat f = PixelFormat::BGR888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);

    u8 buf[64*64*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = f;
    s.width = 64;
    s.height = 64;
    s.pitch = s.width * bpp;
    s.data = buf;

    fillRect(s, 0, 0, { .rgba = {0, 0, 0, 255} }, s.width, s.height);

    {
        profiler_1.beginProfile();
        defer {
            auto pRes = profiler_1.endProfile();
            logInfo("Profiler 1");
            pRes.logResult(core::LogLevel::L_INFO);
        };

        constexpr addr_size N = 5000000;
        core::rndInit();
        for (addr_size i = 0; i < N; i++) {
            i32 ax = i32(core::rndU32() % u32(s.width));
            i32 ay = i32(core::rndU32() % u32(s.height));
            i32 bx = i32(core::rndU32() % u32(s.width));
            i32 by = i32(core::rndU32() % u32(s.height));
            Color color = { .rgba = { u8(core::rndU32()%255), u8(core::rndU32()%255), u8(core::rndU32()%255), u8(core::rndU32()%255) } };

            {
                TIME_BLOCK(profiler_1, PP_DRAW_LINE, "Draw Line");
                fillLine(s, ax, ay, bx, by, color);
            }
        }
    }

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
}

i32 main() {
    [[maybe_unused]] const char* output = OUT_DIRECTORY "/output.tga";
    [[maybe_unused]] const char* depthPathOutput =  OUT_DIRECTORY "/depth-output.tga";
    [[maybe_unused]] const char* inputFile = ASSETS_DIRECTORY "/test_assets/tga/fileformat/ubw8.tga";

    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        Panic(initializeDebugRendering(), "Failed to initialize debug rendering!");
        defer { shutdownDebugRendering(); };

        create5MillionLines(output);
    }
    return 0;
}
