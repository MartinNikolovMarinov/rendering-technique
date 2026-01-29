#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "color.h"

void testOneTGAFile(const char* path, const char* outputPath, bool debugRender = false) {
    logInfo("Parsing File: {}", path);

    auto tgaFile = core::Unpack(TGA::loadFile(path));
    defer { tgaFile.free(); };
    logInfo_TGAFile(tgaFile);

    const TGA::Header* h = nullptr;
    core::Expect(tgaFile.header(h));

    bool imageTypeSupported = h->imageType == 2 || h->imageType == 3;
    if (imageTypeSupported) {
        logInfo("Image type is supported {}", h->imageType);

        Surface surface = core::Unpack(createSurfaceFromTgaImage(tgaFile), "Failed to create surface from TGA file.");
        defer { surface.free(); };
        logInfo_Surface(surface);

        if (outputPath) {
            TGA::CreateFileFromSurfaceParams params = {
                .surface = surface,
                .path = outputPath,
                .imageType = h->imageType,
                .fileType = tgaFile.fileType(),
            };
            core::Expect(TGA::createFileFromSurface(params));
            logInfo("Wrote imageType({}) to file {}", params.imageType, params.path);
        }

        if (debugRender) {
            debug_immPreviewSurface(surface);
        }
    }
    else {
        logWarn("Image type is not supported {}", h->imageType);
    }
}

void testAllTGAFilesInDirectory(const char* directoryPath) {
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

            testOneTGAFile(pathBuffer, nullptr, false);
        }

        return true;
    }, const_cast<char*>(directoryPath));

    if (ret.hasErr()) {
        logErr_PltErrorCode(ret.err());
        PanicFmt(false, "failed to walk direcotry: {}", directoryPath);
    }
}

i32 main() {
    [[maybe_unused]] const char* output = OUT_DIRECTORY "/output.tga";
    [[maybe_unused]] const char* inputFile = ASSETS_DIRECTORY "/test_assets/tga/ftrvxmtrx/monochrome16_top_left.tga";

    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        Panic(initializeDebugRendering(), "Failed to initialize debug rendering!");
        defer { shutdownDebugRendering(); };

        // testAllTGAFilesInDirectory(ASSETS_DIRECTORY "/test_assets/tga/fileformat/");
        testOneTGAFile(inputFile, output, true);
    }

    return 0;
}
