#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"

#include <ctime>

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

            auto surface = core::Unpack(createSurfaceFromTgaFile(tgaFile), "Failed to create surface from TGA file.");
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
    s.origin = Origin::BottomLeft;
    s.pixelFormat = f;
    s.width = 64;
    s.height = 64;
    s.pitch = s.width * bpp;
    s.data = buf;

    fillRect(s, 0, 0, { .rgba = {0, 0, 0, 255} }, s.width, s.height);

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
        std::srand(u32(std::time({})));
        for (addr_size i = 0; i < N; i++) {
            i32 ax = rand() % s.width;
            i32 ay = rand() % s.height;
            i32 bx = rand() % s.width;
            i32 by = rand() % s.height;
            Color color = { .rgba = { u8(rand()%255), u8(rand()%255), u8(rand()%255), u8(rand()%255) } };

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

int main() {
    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };
        // initializeDebugRendering();
        // defer { shutdownDebugRendering(); };

        create5MillionLines(ASSETS_DIRECTORY "/example.tga");
        // createFileTest(ASSETS_DIRECTORY "/example.tga");
        // testAllFilesInDirectory(ASSETS_DIRECTORY "/tga-test-suite/my_test_suite/");
        // testOneFile(ASSETS_DIRECTORY "/example.tga", true);
    }
    return 0;
}
