#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"

void testParseOneFile(const char* path) {
    logInfo("Parsing File: {}", path);

    auto tgaFile = core::Unpack(TGA::loadFile(path));
    defer { tgaFile.free(); };
    logInfo_TGAFile(tgaFile);

    auto surface = core::Unpack(createSurfaceFromTgaFile(tgaFile), "Failed to create surface from TGA file.");
    defer { surface.free(); };
    logInfo_Surface(surface);
}

void testLoadFileAndDebugRender(const char* path) {
    logInfo("Parsing File: {}", path);

    auto tgaFile = core::Unpack(TGA::loadFile(path));
    defer { tgaFile.free(); };
    logInfo_TGAFile(tgaFile);

    auto surface = core::Unpack(createSurfaceFromTgaFile(tgaFile), "Failed to create surface from TGA file.");
    defer { surface.free(); };
    logInfo_Surface(surface);

    debug_immPreviewSurface(surface);
}


void testParseAllFilesInDirectory(const char* directoryPath) {
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

            testParseOneFile(pathBuffer);
        }

        return true;
    }, const_cast<char*>(directoryPath));

    if (ret.hasErr()) {
        logErr_PltErrorCode(ret.err());
        PanicFmt(false, "failed to walk direcotry: {}", directoryPath);
    }
}

int main() {
    coreInit(core::LogLevel::L_DEBUG);

    testLoadFileAndDebugRender(ASSETS_DIRECTORY "/tga-test-suite/uwaterloo/serrano.tga");
    // testParseOneFile(ASSETS_DIRECTORY "/tga-test-suite/uwaterloo/serrano.tga");
    // testAllFilesInAssetsDirectory(ASSETS_DIRECTORY "/tga-test-suite/uwaterloo/");

    coreShutdown();
    return 0;
}
