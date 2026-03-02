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

void renderViewportTestScene(const char* outputPath) {
    constexpr const char* objFiles[] = {
        ASSETS_DIRECTORY "/test_assets/obj/multipart/body.obj",
        ASSETS_DIRECTORY "/test_assets/obj/multipart/head.obj",
        ASSETS_DIRECTORY "/test_assets/obj/multipart/eyes.obj",

        // ASSETS_DIRECTORY "/test_assets/obj/single_file_models/diablo3_pose.obj",
        // ASSETS_DIRECTORY "/test_assets/obj/single_file_models/diablo3_pose.obj",
        // ASSETS_DIRECTORY "/test_assets/obj/single_file_models/diablo3_pose.obj",
    };
    constexpr i32 objFilesLen = CORE_C_ARRLEN(objFiles);

    constexpr i32 WIDTH = 3440;
    constexpr i32 HEIGHT = 1440;
    constexpr i32 VIEWPORT_ROWS = 4;
    constexpr i32 VIEWPORT_COLS = 3;
    constexpr i32 OFFSET_ROWS = WIDTH/VIEWPORT_ROWS;
    constexpr i32 OFFSET_COLS = HEIGHT/VIEWPORT_COLS;

    struct TestTable {
        ViewPort viewport;
        i32 assetIdx;
        bool renderWire;
        bool renderDepth;
        bool renderColor;
    };

    constexpr TestTable testTable[] = {
        // Row 1
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*0, OFFSET_COLS*0), core::v(OFFSET_ROWS*1, OFFSET_COLS*1)),
            .assetIdx = 0,
            .renderWire = false,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*1, OFFSET_COLS*0), core::v(OFFSET_ROWS*2, OFFSET_COLS*1)),
            .assetIdx = 1,
            .renderWire = false,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*2, OFFSET_COLS*0), core::v(OFFSET_ROWS*3, OFFSET_COLS*1)),
            .assetIdx = 2,
            .renderWire = false,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*3, OFFSET_COLS*0), core::v(OFFSET_ROWS*4, OFFSET_COLS*1)),
            .assetIdx = 3,
            .renderWire = false,
            .renderDepth = false,
            .renderColor = true,
        },

        // Row 2
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*0, OFFSET_COLS*1), core::v(OFFSET_ROWS*1, OFFSET_COLS*2)),
            .assetIdx = 0,
            .renderWire = false,
            .renderDepth = true,
            .renderColor = false,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*1, OFFSET_COLS*1), core::v(OFFSET_ROWS*2, OFFSET_COLS*2)),
            .assetIdx = 1,
            .renderWire = false,
            .renderDepth = true,
            .renderColor = false,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*2, OFFSET_COLS*1), core::v(OFFSET_ROWS*3, OFFSET_COLS*2)),
            .assetIdx = 2,
            .renderWire = false,
            .renderDepth = true,
            .renderColor = false,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*3, OFFSET_COLS*1), core::v(OFFSET_ROWS*4, OFFSET_COLS*2)),
            .assetIdx = 3,
            .renderWire = false,
            .renderDepth = true,
            .renderColor = false,
        },

        // Row 3
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*0, OFFSET_COLS*2), core::v(OFFSET_ROWS*1, OFFSET_COLS*3)),
            .assetIdx = 0,
            .renderWire = true,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*1, OFFSET_COLS*2), core::v(OFFSET_ROWS*2, OFFSET_COLS*3)),
            .assetIdx = 1,
            .renderWire = true,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*2, OFFSET_COLS*2), core::v(OFFSET_ROWS*3, OFFSET_COLS*3)),
            .assetIdx = 2,
            .renderWire = true,
            .renderDepth = false,
            .renderColor = true,
        },
        {
            .viewport = ViewPort(core::v(OFFSET_ROWS*3, OFFSET_COLS*2), core::v(OFFSET_ROWS*4, OFFSET_COLS*3)),
            .assetIdx = 3,
            .renderWire = true,
            .renderDepth = false,
            .renderColor = true,
        },
    };

    //==================================================================================================================
    // Initialize Surfaces
    //==================================================================================================================

    Surface outputSurface;
    {
        constexpr PixelFormat pixelFormat = PixelFormat::BGR888;
        constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
        static u8 outbuf[WIDTH*HEIGHT*bpp] = {};
        outputSurface = {
            .actx = nullptr,
            .origin = Origin::TopLeft,
            .pixelFormat = pixelFormat,
            .width = WIDTH,
            .height = HEIGHT,
            .pitch = WIDTH * bpp,
            .data = outbuf,
        };
    }
    defer { outputSurface.free(); };

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

    //==================================================================================================================
    // Read Wavefront Object Files and Create 3D Models
    //==================================================================================================================

    core::ArrStatic<Model3D, objFilesLen> models;
    for (i32 i = 0; i < objFilesLen; i++) {
        Wavefront::WavefrontObj obj = Unpack(Wavefront::loadFile(objFiles[i], Wavefront::WavefrontVersion::VERSION_3_0));
        defer { obj.free(); };
        logInfo("verts={}, faces={}", obj.vertices.at, obj.faces.at);

        Model3D model = Wavefront::createModelFromWavefrontObj(obj, false, true);
        models.push(std::move(model));
    }

    defer {
        for (i32 i = 0; i < i32(models.len()); i++) {
            models[addr_size(i)].free();
        }
    };

    //==================================================================================================================
    // Render
    //==================================================================================================================

    auto& actx = core::getAllocator(core::DEFAULT_ALLOCATOR_ID);
    RendererHandle r = rendererInit(actx);
    defer { rendererDestory(r); };

    for (addr_size i = 0; i < CORE_C_ARRLEN(testTable); i++) {
        auto& [viewport, assetIdx, renderWire, renderDepth, renderColor] = testTable[i];

        {
            rendererSetViewport(r, viewport);
            rendererSetWireframe(r, false);

            rendererSetOutput(r, outputSurface);
            rendererClear(r, BLACK);

            if (renderWire) rendererSetWireframe(r, true);
            defer { if (renderWire) rendererSetWireframe(r, false); };

            i32 modelsCount = 1;
            i32 modelIdx = assetIdx;
            if (assetIdx == 3) {
                modelsCount = 3;
                modelIdx = 0;
            }

            rendererBeginFrame(r);
            {
                for (i32 j = 0; j < modelsCount; j++) {
                    auto& model = models[modelIdx + j];
                    {
                        rendererSetVertexBuffer(r, model.vertices);
                        rendererSetIndexBuffer(r, model.faces);
                        rendererCalculateDepthBuffer(r, depthBuffer);

                        if (renderColor) {
                            rendererColorPass(r);
                        }
                        if (renderDepth) {
                            rendererDepthColorPass(r);
                        }
                    }
                }
            }
            rendererEndFrame(r);

            // Debug render the viewport boundaries:
            strokeViewport(outputSurface, viewport, WHITE);
        }
    }

    //==================================================================================================================
    // Write the Surfaces to Output File
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
}

i32 main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        const char* output = OUT_DIRECTORY "/output.tga";
        renderViewportTestScene(output);
    }
    return 0;
}
