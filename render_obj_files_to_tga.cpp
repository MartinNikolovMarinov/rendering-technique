#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "depth_buffer.h"
#include "wavefront_files.h"
#include "face.h"
#include "model.h"
#include "color.h"

void renderObjFilesToTga(
    const char** objFiles,
    i32 objFilesLen,
    const char* outputPath,
    const char* outputDepth,
    const char * wireFramePath
) {
    //==================================================================================================================
    // Initialize Surfaces
    //==================================================================================================================

    constexpr addr_size WIDTH = 1024;
    constexpr addr_size HEIGHT = 1024;
    constexpr ViewPort VIEW_PORT = ViewPort(core::v(0, 0), core::v(i32(WIDTH), i32(HEIGHT)));

    Surface outputSurface;
    {
        constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
        constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
        static u8 outbuf[WIDTH*HEIGHT*bpp] = {};
        outputSurface = {
            .actx = nullptr,
            .origin = Origin::BottomLeft,
            .pixelFormat = pixelFormat,
            .width = WIDTH,
            .height = HEIGHT,
            .pitch = WIDTH * bpp,
            .data = outbuf,
        };
    }
    defer { outputSurface.free(); };

    Surface wireFrameSurface;
    {
        constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
        constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
        static u8 wbuf[WIDTH*HEIGHT*bpp] = {};
        wireFrameSurface = {
            .actx = nullptr,
            .origin = Origin::BottomLeft,
            .pixelFormat = pixelFormat,
            .width = WIDTH,
            .height = HEIGHT,
            .pitch = WIDTH * bpp,
            .data = wbuf,
        };
    }
    defer { wireFrameSurface.free(); };

    DepthBuffer depthBuffer;
    {
        static f32 depthbuf[WIDTH*HEIGHT] = {};
        depthBuffer = {
            .actx = nullptr,
            .width = WIDTH,
            .height = HEIGHT,
            .data = depthbuf,
        };
        depthBuffer.clear(0.0f);
    }
    defer { depthBuffer.free(); };

    DepthBuffer savedDepthBuffer;
    {
        static f32 savedDepthbuf[WIDTH*HEIGHT] = {};
        savedDepthBuffer = {
            .actx = nullptr,
            .width = WIDTH,
            .height = HEIGHT,
            .data = savedDepthbuf,
        };
        savedDepthBuffer.clear(0.0f);
    }
    defer { savedDepthBuffer.free(); };

    Surface depthBufferVisSurface;
    {
        constexpr PixelFormat pixelFormat = PixelFormat::GRAY8;
        constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
        static u8 depthbufVis[WIDTH*HEIGHT*bpp] = {};
        depthBufferVisSurface = {
            .actx = nullptr,
            .origin = Origin::BottomLeft,
            .pixelFormat = pixelFormat,
            .width = WIDTH,
            .height = HEIGHT,
            .pitch = WIDTH * bpp,
            .data = depthbufVis,
        };
    }
    defer { depthBufferVisSurface.free(); };

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

    // Rasterize
    {
        rendererSetViewport(r, VIEW_PORT);
        rendererSetWireframe(r, false);

        rendererSetOutput(r, outputSurface);
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

            // Save the depth buffer, because renderer end frame will clear it otherwise:
            core::memcopy(savedDepthBuffer.data, depthBuffer.data, addr_size(depthBuffer.size()));
        }
        rendererEndFrame(r);
    }

    // Wireframe
    {
        rendererSetViewport(r, VIEW_PORT);
        rendererSetWireframe(r, true);

        rendererSetOutput(r, wireFrameSurface);
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
        Expect(TGA::createFileFromSurface(params));
        logInfo("Created output file in \"{}\"", outputPath);
    }
    {
        TGA::CreateFileFromSurfaceParams params = {
            .surface = wireFrameSurface,
            .path = wireFramePath,
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        Expect(TGA::createFileFromSurface(params));
        logInfo("Created output file in \"{}\"", outputPath);
    }
    {
        depthBufferToGrayscaleSurface(savedDepthBuffer, depthBufferVisSurface);

        TGA::CreateFileFromSurfaceParams params = {
            .surface = depthBufferVisSurface,
            .path = outputDepth,
            .imageType = 3,
            .fileType = TGA::FileType::New,
        };
        Expect(TGA::createFileFromSurface(params));
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

            // ASSETS_DIRECTORY "/test_assets/obj/simple/rectangle_with_arrow.obj",
        };

        const char* output = OUT_DIRECTORY "/output.tga";
        const char* outputDepth = OUT_DIRECTORY "/output-depth.tga";
        const char* wireFrameOutput = OUT_DIRECTORY "/output-wire.tga";
        renderObjFilesToTga(filesToRender, CORE_C_ARRLEN(filesToRender), output, outputDepth, wireFrameOutput);
    }
    return 0;
}
