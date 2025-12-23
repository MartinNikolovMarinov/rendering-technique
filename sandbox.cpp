#include "core_init.h"
#include "tga_files.h"

#define FILE_PATH "earth.tga"

int main() {
    coreInit(core::LogLevel::L_DEBUG);

    {
        auto tgaFile = core::Unpack(TGA::loadTgaFile(ASSETS_DIRECTORY "/" FILE_PATH));
        defer { tgaFile.free(); };
        logInfo(
            "File Type: {}",
            tgaFile.fileType() == TGA::FileType::New ? "new TGA format" : "original TGA format"
        );
    }

    coreShutdown();
    return 0;
}
