#include "log_utils.h"
#include "surface.h"
#include "tga_files.h"

void logErr_PltErrorCode(core::PltErrCode pltErrCode) {
    char buff[core::MAX_SYSTEM_ERR_MSG_SIZE] = {};
    bool ok = core::pltErrorDescribe(pltErrCode, buff);
    if (ok) {
        logErr("Platform Error; reason: {}", buff);
    }
    else {
        logErr("Platform Error; failed to describe error code: {}", i64(pltErrCode));
    }
}

void logInfo_Surface(const Surface& surface) {
    logInfo(
        "Surface: width={}, height={}, pitch={}",
        surface.width, surface.height, surface.pitch
    );

    // TODO: log the pixel format appropriately: surface.pixelFormat
}

void logInfo_TGAFile(TGA::TGAFile& file) {
    if (file.isValid()) {
        const TGA::Header* header;
        if (auto res = file.header(header); res.hasErr()) {
            logInfo("TGA file has an invalid header");
            return;
        }

        logInfo(
            "\n{{\n"
            "  \"Header\": {{\n"
            "    \"idLength\": {},\n"
            "    \"colorMapType\": {},\n"
            "    \"imageType\": {},\n"
            "    \"colorMapIndex\": {},\n"
            "    \"colorMapLength\": {},\n"
            "    \"colorMapEntrySize\": {},\n"
            "    \"offsetX\": {},\n"
            "    \"offsetY\": {},\n"
            "    \"width\": {},\n"
            "    \"height\": {},\n"
            "    \"pixelDepth\": {},\n"
            "    \"alphaBits\": {},\n"
            "    \"origin\": {}\n"
            "  }}\n"
            "}}",
            header->idLength, header->colorMapType, header->imageType,
            header->colorMapFirstEntryIdx(), header->colorMapLength(), header->colorMapEntrySize(),
            header->offsetX(), header->offsetY(), header->width(), header->height(),
            header->pixelDepth(), header->alphaBits(), header->origin()
        );
    }
    else {
        logInfo("TGA file is not valid");
    }
}
