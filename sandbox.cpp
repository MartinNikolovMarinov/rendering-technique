#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "wavefront_files.h"

// TODO: Write tests.

core::Profiler profiler_1;

enum ProfilePoints {
    PP_RESERVED,

    PP_DRAW_LINE,
};

void testOneFile(const char* path, bool debugRender = false) {
    logInfo("Parsing File: {}", path);

    auto tgaFile = core::Unpack(TGA::loadFile(path));
    defer { tgaFile.free(); };
    logInfo_TGAFile(tgaFile);

    if (debugRender) {
        const TGA::Header* h = nullptr;
        core::Expect(tgaFile.header(h));

        if (h->imageType == 2) {

            auto surface = core::Unpack(createSurfaceFromTgaImage(tgaFile), "Failed to create surface from TGA file.");
            defer { surface.free(); };
            logInfo_Surface(surface);

            logInfo("Image is True Color; rendering is supported.");
            debug_immPreviewSurface(surface);
        }
    }
}

void testAllFilesInDirectory(const char* directoryPath) {
    auto ret = core::dirWalk(directoryPath, [](const core::DirEntry& entry, addr_size, void* userData) -> bool {
        if (entry.type == core::FileType::Regular) {
            const addr_size nameLen = core::cstrLen(entry.name);
            if (entry.name[nameLen - 1] != 't' &&
                entry.name[nameLen - 2] != 'g' &&
                entry.name[nameLen - 3] != 'a'
            ) {
                return true;
            }

            const char* basePath = reinterpret_cast<const char*>(userData);

            static char pathBuffer[1024*1024] = {};
            core::memset(pathBuffer, char(0), 1024*1024);

            addr_size idx = core::memcopy(pathBuffer, basePath, core::cstrLen(basePath));
            core::memcopy(pathBuffer + idx, entry.name, nameLen);

            testOneFile(pathBuffer, true);
        }

        return true;
    }, const_cast<char*>(directoryPath));

    if (ret.hasErr()) {
        logErr_PltErrorCode(ret.err());
        PanicFmt(false, "failed to walk direcotry: {}", directoryPath);
    }
}

void createFileTest(const char* path) {
    constexpr PixelFormat f = PixelFormat::BGRA8888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);

    u8 buf[64*64*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::TopRight;
    s.pixelFormat = f;
    s.width = 64;
    s.height = 64;
    s.pitch = s.width * bpp;
    s.data = buf;

    fillRect(s, 0, 0, { .rgba = {0, 0, 0, 255} }, s.width, s.height);

    fillRect(s, 5, 5, { .rgba = {255, 0, 255, 255} }, s.width - 5, s.height - 5);

    int ax =  7, ay =  3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    fillLine(s, ax, ay, bx, by, BLUE);
    fillLine(s, cx, cy, bx, by, GREEN);
    fillLine(s, cx, cy, ax, ay, YELLOW);
    fillLine(s, ax, ay, cx, cy, RED);

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
}

void create5MillionLines(const char* path) {
    constexpr PixelFormat f = PixelFormat::BGR888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);

    u8 buf[64*64*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = f;
    s.width = 64;
    s.height = 64;
    s.pitch = s.width * bpp;
    s.data = buf;

    fillRect(s, 0, 0, { .rgba = {0, 0, 0, 255} }, s.width, s.height);

    {
        profiler_1.beginProfile();
        defer {
            auto pRes = profiler_1.endProfile();
            logInfo("Profiler 1");
            pRes.logResult(core::LogLevel::L_INFO);
        };

        constexpr addr_size N = 5000000;
        core::rndInit();
        for (addr_size i = 0; i < N; i++) {
            i32 ax = i32(core::rndU32() % u32(s.width));
            i32 ay = i32(core::rndU32() % u32(s.height));
            i32 bx = i32(core::rndU32() % u32(s.width));
            i32 by = i32(core::rndU32() % u32(s.height));
            Color color = { .rgba = { u8(core::rndU32()%255), u8(core::rndU32()%255), u8(core::rndU32()%255), u8(core::rndU32()%255) } };

            {
                TIME_BLOCK(profiler_1, PP_DRAW_LINE, "Draw Line");
                fillLine(s, ax, ay, bx, by, color);
            }
        }
    }

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
}

void renderObjFile(const char* path) {
    auto obj = core::Unpack(Wavefront::loadFile(path, Wavefront::WavefrontVersion::VERSION_3_0));
    defer { obj.free(); };
    logInfo("verts={}, faces={}", obj.verticesCount, obj.facesCount);

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

    auto orthogonalProjection = [](core::vec4f& normVec, i32 width, i32 height) -> core::vec2i {
        i32 ax = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
        i32 ay = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
        return core::v(ax, ay);
    };

    for (i32 i = 0; i < obj.facesCount; i++) {
        auto& f = obj.faces[i];

        i32 vert1Idx = f.v()[0] - 1;
        i32 vert2Idx = f.v()[1] - 1;
        i32 vert3Idx = f.v()[2] - 1;

        core::vec4f& v1 = obj.vertices[vert1Idx];
        core::vec4f& v2 = obj.vertices[vert2Idx];
        core::vec4f& v3 = obj.vertices[vert3Idx];

        core::vec2i a = orthogonalProjection(v1, s.width, s.height);
        core::vec2i b = orthogonalProjection(v2, s.width, s.height);
        core::vec2i c = orthogonalProjection(v3, s.width, s.height);

        // Draw triangle
        strokeTriangle(s, a.x(), a.y(), b.x(), b.y(), c.x(), c.y(), RED);
    }

    for (i32 i = 0; i < obj.verticesCount; i++) {
        auto& v = obj.vertices[i];
        core::vec2i a = orthogonalProjection(v, s.width, s.height);
        fillPixel(s, a.x(), a.y(), WHITE);
    }

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = ASSETS_DIRECTORY "/output.tga",
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(params));
}

int main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };
        // Panic(initializeDebugRendering(), "Failed to initialize debug rendering!");
        // defer { shutdownDebugRendering(); };

        // renderObjFile(ASSETS_DIRECTORY "/obj-files/simple/floor.obj");
        renderObjFile(ASSETS_DIRECTORY "/obj-files/diablo3_pose.obj");

        // create5MillionLines(ASSETS_DIRECTORY "/output.tga");
        // createFileTest(ASSETS_DIRECTORY "/output.tga");

        // testAllFilesInDirectory(ASSETS_DIRECTORY "/tga-test-suite/my_test_suite/");
        // testOneFile(ASSETS_DIRECTORY "/output.tga", true);
    }
    return 0;
}
