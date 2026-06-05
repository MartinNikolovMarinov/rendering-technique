#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "depth_buffer.h"
#include "wavefront_files.h"
#include "model.h"
#include "color.h"

void renderObjFilesToTga(const char** objFiles, i32 objFilesLen, const char* outputDir) {
    //==================================================================================================================
    // Initialize Surfaces
    //==================================================================================================================

    constexpr addr_size WIDTH = 1024;
    constexpr addr_size HIGHT = 1024;
    constexpr ViewPort VIEW_PORT = ViewPort(core::v(0, 0), core::v(i32(WIDTH), i32(HIGHT)));

    Surface outputSurface;
    {
        constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
        constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
        static u8 outbuf[WIDTH*HIGHT*bpp] = {};
        outputSurface = {
            .actx = nullptr,
            .origin = Origin::BottomLeft,
            .pixelFormat = pixelFormat,
            .width = WIDTH,
            .height = HIGHT,
            .pitch = WIDTH * bpp,
            .data = outbuf,
        };
    }
    defer { outputSurface.free(); };

    DepthBuffer depthBuffer;
    {
        static f32 depthbuf[WIDTH*HIGHT] = {};
        depthBuffer = {
            .actx = nullptr,
            .width = WIDTH,
            .height = HIGHT,
            .data = depthbuf,
        };
        depthBuffer.clear(0.0f);
    }
    defer { depthBuffer.free(); };

    //==================================================================================================================
    // Read Wavefront Object Files and Create 3D Models
    //==================================================================================================================

    core::ArrStatic<Model3D, 10> models;
    for (i32 i = 0; i < objFilesLen; i++) {
        Wavefront::WavefrontObj obj = Unpack(Wavefront::loadFile(objFiles[i], Wavefront::WavefrontVersion::VERSION_3_0));
        defer { obj.free(); };
        logInfo("verts={}, faces={}", obj.vertices.at, obj.faces.at);

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

    debug_rendererOutputFrameToFile(r, outputDir);

    rendererSetViewport(r, VIEW_PORT);
    rendererSetOutput(r, outputSurface);

    // Rasterize
    {
        rendererSetWireframe(r, false);

        rendererClear(r, BLACK);

        rendererBeginFrame(r);
        {
            for (i32 i = 0; i < objFilesLen; i++) {
                auto& model = models[addr_size(i)];
                rendererSetVertexBuffer(r, model.vertices);
                rendererSetIndexBuffer(r, model.faces);
                rendererCalculateDepthBuffer(r, depthBuffer);
                rendererColorPass(r);
            }
        }
        rendererEndFrame(r);
    }

    // Wireframe
    {
        rendererSetWireframe(r, true);

        rendererClear(r, BLACK);

        rendererBeginFrame(r);
        {
            for (i32 i = 0; i < objFilesLen; i++) {
                auto& model = models[addr_size(i)];

                rendererSetVertexBuffer(r, model.vertices);
                rendererSetIndexBuffer(r, model.faces);
                rendererColorPass(r);
            }
        }
        rendererEndFrame(r);
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

            // ASSETS_DIRECTORY "/test_assets/obj/simple/rectangle_with_arrow.obj",
        };

        renderObjFilesToTga(filesToRender, CORE_C_ARRLEN(filesToRender), OUT_DIRECTORY);
    }
    return 0;
}
