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

void logErr_ConvErrorCode(core::ConversionError convErrCode) {
    const char* errMg = core::conversionErrorToCstr(convErrCode);
    logErr("Conversion Error; reason: {}", errMg);
}

namespace {

template<addr_size Dim, typename T>
void logInfoVectorFloat(core::vec<Dim, T> v) {
    constexpr const char* fmt = core::same_as<T, f64> ? "\"{}\": {:f.6}, " : "\"{}\": {:f.3}, ";
    constexpr const char* fmtLast = core::same_as<T, f64> ? "\"{}\": {:f.6}" : "\"{}\": {:f.3}";

    constexpr addr_size bufferLen = 255;
    char buff[bufferLen] = {};
    char* p = buff;

    constexpr const char* prefix = "{{ \"vec{}{}\": {{";
    p += core::Unpack(
        core::format(p, bufferLen, prefix, v.dimensions(), core::same_as<T, f64> ? 'd' : 'f'),
        "BUG: likely buffer overflow"
    );

    constexpr char symbols[] = { 'x', 'y', 'z', 'w' };
    for (addr_size i = 0; i < v.dimensions(); i++) {
        if (i < v.dimensions() - 1) {
            p += core::Unpack(core::format(p, bufferLen, fmt, symbols[i], v.data[i]), "BUG: likely buffer overflow");
        }
        else {
            p += core::Unpack(core::format(p, bufferLen, fmtLast, symbols[i], v.data[i]), "BUG: likely buffer overflow");
        }
    }

    constexpr const char* postfix = "} }";
    p += core::memcopy(p, postfix, core::cstrLen(postfix));

    logInfo("{}", buff);
}

} // namespace

void logInfo_Vector(core::vec4f v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec3f v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec2f v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec1f v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec4d v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec3d v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec2d v) { logInfoVectorFloat(v); }
void logInfo_Vector(core::vec1d v) { logInfoVectorFloat(v); }

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

void logInfo_TGAFile(TGA::TGAImage& file) {
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
