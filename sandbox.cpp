#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"

#define FILE_PATH "earth.tga"

int main() {
    coreInit(core::LogLevel::L_DEBUG);

    {
        auto tgaFile = core::Unpack(TGA::loadFile(ASSETS_DIRECTORY "/" FILE_PATH));
        defer { tgaFile.free(); };
        logInfo(
            "File Type: {}",
            tgaFile.fileType() == TGA::FileType::New ? "new TGA format" : "original TGA format"
        );

        auto surface = core::Unpack(createSurfaceFromTgaFile(tgaFile), "Failed to create surface from TGA file.");
        logInfo_Surface(surface);
    }

    coreShutdown();
    return 0;
}
