#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "wavefront_files.h"
#include "face.h"
#include "model.h"
#include "color.h"

void renderObjFilesToTga(const char** objFiles, i32 objFilesLen, const char* outputPath, const char* outputDepth) {
    //==================================================================================================================
    // Initialize Surfaces
    //==================================================================================================================

    constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
    constexpr addr_size WIDTH = 1024;
    constexpr addr_size HEIGHT = 1024;
    constexpr bool wireFrameMode = false;

    static u8 outbuf[WIDTH*HEIGHT*bpp] = {};
    Surface outputSurface = {
        .actx = nullptr,
        .origin = Origin::BottomLeft,
        .pixelFormat = pixelFormat,
        .width = WIDTH,
        .height = HEIGHT,
        .pitch = WIDTH * bpp,
        .data = outbuf,
    };
    defer { outputSurface.free(); };

    static u8 depthbuf[WIDTH*HEIGHT*bpp] = {};
    Surface depthBuffer = {
        .actx = nullptr,
        .origin = Origin::BottomLeft,
        .pixelFormat = pixelFormat,
        .width = WIDTH,
        .height = HEIGHT,
        .pitch = WIDTH * bpp,
        .data = depthbuf,
    };
    defer { depthBuffer.free(); };

    //==================================================================================================================
    // Read Wavefront Object Files and Create 3D Models
    //==================================================================================================================

    core::ArrStatic<Model3D, 10> models;
    for (i32 i = 0; i < objFilesLen; i++) {
        Wavefront::WavefrontObj obj = core::Unpack(Wavefront::loadFile(objFiles[i], Wavefront::WavefrontVersion::VERSION_3_0));
        defer { obj.free(); };
        logInfo("verts={}, faces={}", obj.verticesCount, obj.facesCount);

        Model3D model = Wavefront::createModelFromWavefrontObj(obj);
        models.push(std::move(model));
    }

    defer {
        for (i32 i = 0; i < objFilesLen; i++) {
            models[addr_size(i)].free();
        }
    };

    //==================================================================================================================
    // Render
    //==================================================================================================================

    auto& actx = core::getAllocator(core::DEFAULT_ALLOCATOR_ID);
    RendererHandle r = rendererInit(actx);
    defer { rendererDestory(r); };

    rendererSetFrameBuffer(r, WIDTH, HEIGHT);
    rendererSetWireframe(r, wireFrameMode);

    rendererSetOutput(r, outputSurface);
    rendererClear(r, BLACK);

    // Clear rendering target surface:
    for (i32 i = 0; i < objFilesLen; i++) {
        auto& model = models[addr_size(i)];

        rendererBeginFrame(r);
        {
            rendererSetVertexBuffer(r, model.vertices);
            rendererSetIndexBuffer(r, model.faces);
            rendererCalculateDepthBuffer(r, depthBuffer);
        }
        rendererEndFrame(r);
    }

    //==================================================================================================================
    // Write the Surfaces to Output Files
    //==================================================================================================================

    {
        TGA::CreateFileFromSurfaceParams params = {
            .surface = outputSurface,
            .path = outputPath,
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(params));
        logInfo("Created output file in \"{}\"", outputPath);
    }
    {
        TGA::CreateFileFromSurfaceParams params = {
            .surface = depthBuffer,
            .path = outputDepth,
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(params));
        logInfo("Created depth output file in \"{}\"", outputDepth);
    }
}

i32 main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        const char* filesToRender[] = {
            // ASSETS_DIRECTORY "/test_assets/obj/single_file_models/diablo3_pose.obj",

            ASSETS_DIRECTORY "/test_assets/obj/single_file_models/african_head.obj",

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
