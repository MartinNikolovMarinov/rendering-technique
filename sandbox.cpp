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

void writeSurfaceToFile(const char* path) {
    constexpr PixelFormat f = PixelFormat::BGRA8888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);
    constexpr addr_size WIDTH = 800;
    constexpr addr_size HEIGHT = 800;

    u8 buf[WIDTH*HEIGHT*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = f;
    s.width = WIDTH;
    s.height = HEIGHT;
    s.pitch = s.width * bpp;
    s.data = buf;

    fillRect(s, 0, 0, BLACK, s.width, s.height);

    constexpr f32 scale = 0.3f;
    i32 ax = 290, ay = 170;
    i32 bx = 500, by = 240;
    i32 cx = 130, cy = 650;
    auto a = core::v(ax, ay, 1);
    auto b = core::v(bx, by, 1);
    auto c = core::v(cx, cy, 1);

    u8 buf2[WIDTH*HEIGHT*bpp] = {};
    Surface depthBuffer = Surface();
    depthBuffer.actx = nullptr;
    depthBuffer.origin = Origin::BottomLeft;
    depthBuffer.pixelFormat = f;
    depthBuffer.width = WIDTH;
    depthBuffer.height = HEIGHT;
    depthBuffer.pitch = depthBuffer.width * bpp;
    depthBuffer.data = buf2;

    fillTriangle(s, depthBuffer, a, b, c, RED, GREEN, BLUE);
    strokeTriangleInset(s, depthBuffer, a, b, c, BLUE, RED, GREEN, scale);

    // Outline inner triangle
    {
        const f32 centerX = (f32(a.x()) + f32(b.x()) + f32(c.x())) / 3.0f;
        const f32 centerY = (f32(a.y()) + f32(b.y()) + f32(c.y())) / 3.0f;
        core::vec2i aInner = core::v(
            i32(centerX + (f32(a.x()) - centerX) * (1.0f - scale)),
            i32(centerY + (f32(a.y()) - centerY) * (1.0f - scale))
        );
        core::vec2i bInner = core::v(
            i32(centerX + (f32(b.x()) - centerX) * (1.0f - scale)),
            i32(centerY + (f32(b.y()) - centerY) * (1.0f - scale))
        );
        core::vec2i cInner = core::v(
            i32(centerX + (f32(c.x()) - centerX) * (1.0f - scale)),
            i32(centerY + (f32(c.y()) - centerY) * (1.0f - scale))
        );
        strokeTriangleFast(s, aInner, bInner, cInner, WHITE);
    }

    // Outline outer triangle:
    strokeTriangleFast(s, a.xy(), b.xy(), c.xy(), WHITE);

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
    logInfo("Wrote to file {}", path);
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

        writeSurfaceToFile(output);
    }
    return 0;
}
