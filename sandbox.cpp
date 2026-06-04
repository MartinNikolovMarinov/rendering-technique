#include "color.h"
#include "core_init.h"
#include "surface_renderer.h"
#include "surface.h"
#include "tga_files.h"

i32 main() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    //==================================================================================================================
    // Create surface
    //==================================================================================================================

    constexpr addr_size WIDTH = 1920;
    constexpr addr_size HEIGHT = 1080;
    constexpr PixelFormat PX_FORMAT = PixelFormat::BGRA8888;
    constexpr addr_size BPP = pixelFormatBytesPerPixel(PX_FORMAT);
    static u8 data[WIDTH * HEIGHT * BPP] = {};
    Surface surface = {
        .actx = nullptr,
        .origin = Origin::BottomLeft,
        .pixelFormat = PX_FORMAT,
        .width = WIDTH,
        .height = HEIGHT,
        .pitch = WIDTH * BPP,
        .data = data
    };
    defer { surface.free(); };

    //==================================================================================================================
    // Render into surface
    //==================================================================================================================

    {
        fillRect(surface, 0, 0, WHITE, surface.width, surface.height);

        ViewPort vp = {
            core::v(500, 500),
            core::v(700, 700),
        };
        strokeViewport(surface, vp, BLACK);
        fillLineLocal(surface, vp, -10, 100, 300, 100, RED);
        strokeTriangleFastLocal(surface, vp, core::v(0, 0), core::v(300, 300), core::v(600, 500), BLACK);
        fillTriangleLocal(surface, vp, core::v(0, 0), core::v(300, 300), core::v(600, 500), RED, GREEN, BLUE);
    }

    //==================================================================================================================
    // Write surface to TGA file
    //==================================================================================================================

    constexpr const char* outputFile = OUT_DIRECTORY "/output.tga";
    TGA::CreateFileFromSurfaceParams params = {
        .surface = surface,
        .path = outputFile,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };

    auto res = TGA::createFileFromSurface(params);
    AssertFmt(!res.hasErr(), "Failed to write file: \"{}\"", outputFile);

    logInfo("Done");
    return 0;
}
