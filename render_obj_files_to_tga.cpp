#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "wavefront_files.h"
#include "model.h"
#include "color.h"

void renderObjFilesToTga(const char** objFiles, i32 objFilesLen, const char* outputPath, const char* outputDepth) {
    constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);

    constexpr addr_size WIDTH = 1024;
    constexpr addr_size HEIGHT = 1024;

    static u8 buf[WIDTH*HEIGHT*bpp] = {}; // This might be big
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = pixelFormat;
    s.width = WIDTH;
    s.height = HEIGHT;
    s.pitch = s.width * bpp;
    s.data = buf;

    static u8 buf2[WIDTH*HEIGHT*bpp] = {}; // This might be big
    Surface depthBuffer = Surface();
    depthBuffer.actx = nullptr;
    depthBuffer.origin = Origin::BottomLeft;
    depthBuffer.pixelFormat = pixelFormat;
    depthBuffer.width = WIDTH;
    depthBuffer.height = HEIGHT;
    depthBuffer.pitch = depthBuffer.width * bpp;
    depthBuffer.data = buf2;

    // Clear rendering target surface:
    fillRect(s, 0, 0, BLACK, s.width, s.height);

    for (i32 i = 0; i < objFilesLen; i++) {
        constexpr bool wireframe = false;
        rendererBeginFrame(s.width, s.height, depthBuffer, wireframe);

        auto obj = core::Unpack(Wavefront::loadFile(objFiles[i], Wavefront::WavefrontVersion::VERSION_3_0));
        defer { obj.free(); };
        logInfo("verts={}, faces={}", obj.verticesCount, obj.facesCount);

        auto model = Wavefront::createModelFromWavefrontObj(obj);
        defer { model.free(); };

        renderModel(model);

        rendererEndFrame(s);
    }

    {
        TGA::CreateFileFromSurfaceParams params = {
            .surface = s,
            .path = outputPath,
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(params));
        logInfo("Create an output file in \"{}\"", outputPath);
    }
    {
        TGA::CreateFileFromSurfaceParams params = {
            .surface = depthBuffer,
            .path = outputDepth,
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(params));
        logInfo("Create a depth output file in \"{}\"", outputDepth);
    }
}

i32 main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        const char* filesToRender[] = {
            ASSETS_DIRECTORY "/test_assets/obj/single_file_models/diablo3_pose.obj",

            // ASSETS_DIRECTORY "/test_assets/obj/single_file_models/african_head.obj",

            // ASSETS_DIRECTORY "/test_assets/obj/multipart/body.obj",
            // ASSETS_DIRECTORY "/test_assets/obj/multipart/head.obj",
            // ASSETS_DIRECTORY "/test_assets/obj/multipart/eyes.obj",
        };
        const char* output = OUT_DIRECTORY "/output.tga";
        const char* outputDepth = OUT_DIRECTORY "/output-depth.tga";
        renderObjFilesToTga(filesToRender, CORE_C_ARRLEN(filesToRender), output, outputDepth);
    }
    return 0;
}
