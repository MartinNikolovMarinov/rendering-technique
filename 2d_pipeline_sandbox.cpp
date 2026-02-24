#include "core_init.h"

#include <math/core_projections.h>

#include "wavefront_files.h"
#include "surface.h"
#include "tga_files.h"
#include "surface_renderer.h"
#include "color.h"
#include "log_utils.h"

const char* INPUT_OBJ = ASSETS_DIRECTORY "/test_assets/obj/simple/pipeline_2d_homogeneous.obj";
const char* OUTPUT_TGA_FILE = OUT_DIRECTORY "/output.tga";

constexpr PixelFormat OUTPUT_PIXEL_FORMAT = PixelFormat::BGR888;
constexpr i32 OUTPUT_BPP = pixelFormatBytesPerPixel(OUTPUT_PIXEL_FORMAT);
constexpr addr_size OUTPUT_SURFACE_WIDTH = 1280;
constexpr addr_size OUTPUT_SURFACE_HEIGHT = 720;

static u8 outputSurfaceData[OUTPUT_SURFACE_WIDTH * OUTPUT_SURFACE_HEIGHT * OUTPUT_BPP] = {};
static Surface outputSurface = {
    .actx = nullptr,
    .origin = Origin::BottomLeft,
    .pixelFormat = OUTPUT_PIXEL_FORMAT,
    .width = OUTPUT_SURFACE_WIDTH,
    .height = OUTPUT_SURFACE_HEIGHT,
    .pitch = OUTPUT_SURFACE_WIDTH * OUTPUT_BPP,
    .data = outputSurfaceData,
};

constexpr core::Bbox2D<f32> WORLD_BBOX (core::v(-2.f, -2.f), core::v(2.f, 2.f));

struct Pipeline2D {
    core::mat4x4f model;
    core::mat4x4f view;
    core::mat4x4f projection;
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

Wavefront::WavefrontObj loadWavefrontObj(const char* path) {
    Wavefront::WavefrontObj obj = Unpack(Wavefront::loadFile(path, Wavefront::WavefrontVersion::VERSION_3_0));
    logInfo("Loaded OBJ: \"{}\"", path);
    logInfo("Vertex count: {}", obj.vertices.at);
    logInfo("Face count: {}", obj.faces.at);
    return obj;
}

Pipeline2D createDefault2DPipeline() {
    Pipeline2D ret = {};
    ret.model = core::mat4f::identity();
    ret.view = core::mat4f::identity();

    // Keep the visible world box simple for now: x,y in [-2, 2].
    ret.projection = core::orthoRH_NO(
        WORLD_BBOX.min.x(), WORLD_BBOX.max.x(),   // left, right
        WORLD_BBOX.min.y(), WORLD_BBOX.max.y(),   // bottom, top
        -1.0f, 1.0f                               // near, far
    );
    return ret;
}

core::vec4f worldToClip(const Pipeline2D& p, const core::vec3f& worldPos) {
    core::vec4f worldH = core::v(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
    return p.projection * p.view * p.model * worldH;
}

core::vec2i ndcToScreen(const core::vec3f& ndc, i32 width, i32 height) {
    i32 x = i32((ndc.x() + 1.0f) * 0.5f * f32(width - 1));
    i32 y = i32((ndc.y() + 1.0f) * 0.5f * f32(height - 1));
    return core::v(x, y);
}

core::vec2i worldToScreen(const Pipeline2D& p, const core::vec3f& worldPos, i32 width, i32 height) {
    core::vec4f clip = worldToClip(p, worldPos);

    // For perspective later, this divide is required.
    // For orthographic now, w is 1, so this is effectively a no-op.
    if (core::absGeneric(clip.w()) < 1e-6f) {
        return core::v(0, 0);
    }

    core::vec3f ndc = clip.xyz() / clip.w();
    return ndcToScreen(ndc, width, height);
}

void drawLineWorld(
    Surface& s,
    const Pipeline2D& p,
    const core::vec3f& aWorld,
    const core::vec3f& bWorld,
    const Color& color
) {
    core::vec2i a = worldToScreen(p, aWorld, s.width, s.height);
    core::vec2i b = worldToScreen(p, bWorld, s.width, s.height);
    fillLine(s, a.x(), a.y(), b.x(), b.y(), color);
}

i32 main() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Keep loading in place so this sandbox still validates OBJ parsing.
    auto obj = loadWavefrontObj(INPUT_OBJ);
    defer { obj.free(); };

    fillRect(outputSurface, 0, 0, BLACK, outputSurface.width, outputSurface.height);

    Pipeline2D pipeline = createDefault2DPipeline();

    // STEP 1: Define debug geometry in world coordinates (not in screen pixels).
    core::vec3f worldOrigin = core::v(0.0f, 0.0f, 0.0f);
    core::vec3f worldAxisPositiveX = core::v(WORLD_BBOX.max.x(), 0.0f, 0.0f);
    core::vec3f worldAxisNegativeX = core::v(WORLD_BBOX.min.x(), 0.0f, 0.0f);
    core::vec3f worldAxisPositiveY = core::v(0.0f, WORLD_BBOX.max.y(), 0.0f);
    core::vec3f worldAxisNegativeY = core::v(0.0f, WORLD_BBOX.min.y(), 0.0f);

    // STEP 2 + STEP 3:
    // world -> clip -> ndc -> screen is handled by worldToScreen(),
    // then we draw the axes with the same path your objects will use later.
    drawLineWorld(outputSurface, pipeline, worldOrigin, worldAxisPositiveX, RED);
    drawLineWorld(outputSurface, pipeline, worldOrigin, worldAxisNegativeX, RED);
    drawLineWorld(outputSurface, pipeline, worldOrigin, worldAxisPositiveY, GREEN);
    drawLineWorld(outputSurface, pipeline, worldOrigin, worldAxisNegativeY, GREEN);

    core::vec2i originPixel = worldToScreen(pipeline, worldOrigin, outputSurface.width, outputSurface.height);
    fillPixel(outputSurface, originPixel.x(), originPixel.y(), WHITE);

    writeOutputSurface();

    logInfo("Done");
    return 0;
}
