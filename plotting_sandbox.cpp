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

i32 main() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    auto obj = loadWavefrontObj(INPUT_OBJ);
    defer { obj.free(); };

    clearOutputSurface();

    ViewPort<i32> viewport = { core::v(100, 100), core::v(200, 300) };
    strokeBBox(outputSurface, viewport, WHITE);

    for (i32 y = viewport.min.y(); y < viewport.max.y(); y++) {
        for (i32 x = viewport.min.x(); x < viewport.max.x(); x++) {
            fillPixel(outputSurface, viewport, x, y, GREEN);
        }
    }

    writeOutputSurface();

    logInfo("Done");
    return 0;
}
