#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"

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
    u8 buf[64*64*3] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.pixelFormat = PixelFormat::BGR888;
    s.width = 64;
    s.height = 64;
    s.pitch = s.width * 3;
    s.data = buf;

    u8 b = 255;
    u8 g = 0;
    u8 r = 0;

    for (i32 row = 0; row < s.height; row++) {
        for (i32 col = 0; col < s.width; col++) {
            i32 idx = row * s.pitch + col * 3;
            s.data[idx + 0] = b;
            s.data[idx + 1] = g;
            s.data[idx + 2] = r;
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

        createFileTest(ASSETS_DIRECTORY "/example.tga");

        // testAllFilesInDirectory(ASSETS_DIRECTORY "/tga-test-suite/my_test_suite/");
        // testOneFile(ASSETS_DIRECTORY "/example.tga", true);
    }
    return 0;
}
