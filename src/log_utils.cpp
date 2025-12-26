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
        "\n{{\n"
        "  \"surface\": {{\n"
        "    \"width\": {},\n"
        "    \"height\": {},\n"
        "    \"pitch\": {},\n"
        "    \"pixelFormat\": \"{}\",\n"
        "    \"size\": {},\n"
        "    \"isOwner\": {}\n"
        "  }}\n"
        "}}",
        surface.width,
        surface.height,
        surface.pitch,
        pixelFormatToCstr(surface.pixelFormat),
        surface.size(),
        surface.isOwner()
    );
}

void logInfo_TGAFile(TGA::TGAFile& file) {
    if (file.isValid()) {
        const TGA::Header* header = nullptr;
        if (auto res = file.header(header); res.hasErr()) {
            logInfo("TGA file has an invalid header");
            return;
        }

        const TGA::Footer defaultFooter = {};
        const TGA::Footer* footer = nullptr;
        if (auto res = file.footer(footer); res.hasErr()) {
            footer = &defaultFooter;
        }

        logInfo(
            "\n{{\n"
            "  \"fileType: \"{}\",\n"
            "  \"header\": {{\n"
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
            "  }},\n"
            "  \"footer\": {{\n"
            "    \"developerOffset\": {},\n"
            "    \"extensionOffset\": {},\n"
            "    \"signature\": \"{}\"\n"
            "  }}\n"
            "}}",
            file.fileType() == TGA::FileType::New ? "new" : "original",
            header->idLength, header->colorMapType, header->imageType,
            header->colorMapFirstEntryIdx(), header->colorMapLength(), header->colorMapEntrySize(),
            header->offsetX(), header->offsetY(), header->width(), header->height(),
            header->pixelDepth(), header->alphaBits(), header->origin(),
            file.developerAreaOff, file.extAreaOff, footer->signature
        );
    }
    else {
        logInfo("TGA file is not valid");
    }
}
