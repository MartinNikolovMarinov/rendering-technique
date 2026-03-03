#include "core_init.h"

#include <math/core_projections.h>
#include <math/core_transforms.h>

#include "wavefront_files.h"
#include "surface.h"
#include "tga_files.h"
#include "surface_renderer.h"
#include "color.h"
#include "log_utils.h"

const char* INPUT_OBJ = ASSETS_DIRECTORY "/test_assets/obj/simple/pipeline_2d_transformation_example.obj";
const char* OUTPUT_TGA_FILE = OUT_DIRECTORY "/output.tga";

constexpr PixelFormat OUTPUT_PIXEL_FORMAT = PixelFormat::BGR888;
constexpr i32 OUTPUT_BPP = pixelFormatBytesPerPixel(OUTPUT_PIXEL_FORMAT);
constexpr addr_size OUTPUT_SURFACE_WIDTH = 1280;
constexpr addr_size OUTPUT_SURFACE_HEIGHT = 720;

static u8 outputSurfaceData[OUTPUT_SURFACE_WIDTH * OUTPUT_SURFACE_HEIGHT * OUTPUT_BPP] = {};
static Surface outputSurface = {
    .actx = nullptr,
    .origin = Origin::TopLeft,
    .pixelFormat = OUTPUT_PIXEL_FORMAT,
    .width = OUTPUT_SURFACE_WIDTH,
    .height = OUTPUT_SURFACE_HEIGHT,
    .pitch = OUTPUT_SURFACE_WIDTH * OUTPUT_BPP,
    .data = outputSurfaceData,
};

void writeOutputSurface() {
    TGA::CreateFileFromSurfaceParams params = {
        .surface = outputSurface,
        .path = OUTPUT_TGA_FILE,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    Expect(TGA::createFileFromSurface(params));
}

void clearOutputSurface() {
    fillRect(outputSurface, 0, 0, BLACK, outputSurface.width, outputSurface.height);
}

Wavefront::WavefrontObj loadWavefrontObj(const char* path) {
    Wavefront::WavefrontObj obj = Unpack(Wavefront::loadFile(path, Wavefront::WavefrontVersion::VERSION_3_0));
    logInfo("Loaded OBJ: \"{}\"", path);
    logInfo("Vertex count: {}", obj.vertices.at);
    logInfo("Face count: {}", obj.faces.at);
    return obj;
}

//======================================================================================================================
// Plotting
//======================================================================================================================

struct Plot {

};

template<i32 N>
struct PlottingScene {
    Plot plots[N];

    PlottingScene() : plots({}) {}
};

void debugDrawLinesToViewport(ViewPort viewport) {
    i32 width = (viewport.width() - 1);
    i32 height = (viewport.height() - 1);

    fillLineLocal(outputSurface, viewport, width / 2, 0, width / 2, height, GREEN);
    fillLineLocal(outputSurface, viewport, 0, height / 2, width, height / 2, GREEN);
    fillLineLocal(outputSurface, viewport, 0, 0, width, height, GREEN);
    fillLineLocal(outputSurface, viewport, width, 0, 0, height, GREEN);
}

void debugDrawRects(ViewPort viewport) {
    i32 width = (viewport.width() - 1);
    i32 height = (viewport.height() - 1);

    i32 rect1Width = width / 2;
    i32 rect1Height = height / 2;

    strokeRectLocal(outputSurface, viewport, 2, 2, BLUE, rect1Width + 2, rect1Height + 2);
    fillRectLocal(outputSurface, viewport, 3, 3, GREEN, rect1Width, rect1Height);
}

i32 main() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // auto obj = loadWavefrontObj(INPUT_OBJ);
    // defer { obj.free(); };

    clearOutputSurface();

    ViewPort viewport = {
        core::v(0, 10),
        core::v(i32(OUTPUT_SURFACE_WIDTH), 200)
    };
    ViewPort viewport2 = {
        core::v(i32(OUTPUT_SURFACE_WIDTH) - 10, i32(OUTPUT_SURFACE_HEIGHT) - 10),
        core::v(i32(OUTPUT_SURFACE_WIDTH), i32(OUTPUT_SURFACE_HEIGHT)),
    };
    ViewPort viewport3 = {
        core::v(300, 300),
        core::v(340, 340),
    };
    ViewPort viewport4 = {
        core::v(350, 350),
        core::v(391, 391),
    };
    strokeViewport(outputSurface, viewport, WHITE);
    strokeViewport(outputSurface, viewport2, WHITE);
    strokeViewport(outputSurface, viewport3, WHITE);
    strokeViewport(outputSurface, viewport4, WHITE);

    // for (i32 y = viewport.min.y(); y < viewport.max.y(); y++) {
    //     for (i32 x = viewport.min.x(); x < viewport.max.x(); x++) {
    //         fillPixel(outputSurface, x, y, GREEN);
    //     }
    // }

    debugDrawRects(viewport);
    debugDrawRects(viewport2);
    debugDrawRects(viewport3);
    debugDrawRects(viewport4);

    // fillLine(outputSurface,
    //     viewport.width() / 2, 100, viewport.width() / 2, 100 + viewport.height() - 1,
    //     RED);

    // for (i32 y = 0; y < viewport.height(); y++) {
    //     for (i32 x = 0; x < viewport.width(); x++) {
    //         fillPixelLocal(outputSurface, viewport, x, y, BLUE);
    //     }
    // }

    writeOutputSurface();

    logInfo("Done");
    return 0;
}
