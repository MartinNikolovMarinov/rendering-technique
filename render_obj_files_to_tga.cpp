#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "wavefront_files.h"
#include "model.h"

void renderObjFileIntoASurface(Surface& s, const char* objFilePath, bool wireframe) {
    auto obj = core::Unpack(Wavefront::loadFile(objFilePath, Wavefront::WavefrontVersion::VERSION_3_0));
    logInfo("verts={}, faces={}", obj.verticesCount, obj.facesCount);

    auto model = Wavefront::createModelFromWavefrontObj(obj);
    obj.free();

    renderModel(s, model, wireframe);
    model.free();
}

void renderObjFilesToTga(const char** objFiles, i32 objFilesLen, const char* outputPath) {
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

    fillRect(s, 0, 0, BLACK, s.width, s.height);

    for (i32 i = 0; i < objFilesLen; i++) {
        renderObjFileIntoASurface(s, objFiles[i], false);
    }

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = outputPath,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
}

i32 main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        const char* filesToRender[] = {
            ASSETS_DIRECTORY "/obj-files/diablo3_pose.obj",

            // ASSETS_DIRECTORY "/obj-files/african_head.obj",

            // ASSETS_DIRECTORY "/obj-files/multipart/body.obj",
            // ASSETS_DIRECTORY "/obj-files/multipart/head.obj",
            // ASSETS_DIRECTORY "/obj-files/multipart/eyes.obj",
        };
        const char* output = ASSETS_DIRECTORY "/output.tga";
        renderObjFilesToTga(filesToRender, CORE_C_ARRLEN(filesToRender), output);
    }
    return 0;
}
