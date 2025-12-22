#include "core_init.h"

namespace TGA {

using TGALong = u32;
using TGAShort = u16;
using TGAByte = u8;

enum struct TGAError {
    Undefined,
    InvalidFileFormat,
    OldFormat,
    SENTINEL
};

struct Header {};

struct ImageColorMapData {};

struct DeveloperArea {};

struct ExtensionArea {};

PACK_PUSH
struct PACKED Footer {
    TGALong extensionAreaOffset;      // Bytes 0-3: The Extension Area Offset
    TGALong developerDirectoryOffset; // Bytes 4-7: The Developer Directory Offset
    char signature[16];               // Bytes 8-23: The Signature
    char dot;                         // Byte 24: ASCII Character “.”
    char zeroTerm;                    // Byte 25: Binary zero string terminator (0x00)
};
PACK_POP

enum struct TGAFileType {
    Undefined,
    OriginalTga,
    NewTga,
    SENTINEL
};

struct TGAFile {
    struct OriginalTGAFile {
        Header header;
        ImageColorMapData imageColorMapData;
        Footer footer;
    };

    struct NewTGAFile {
        DeveloperArea devArea;
        ExtensionArea extArea;
    };

    TGAFileType fileType;
    union {
        OriginalTGAFile originalFormat;
        NewTGAFile newFormat;
    };
};

constexpr bool hasSignature(char signature[16]) {
    return core::memcmp(signature, 16, "TRUEVISION-XFILE", 16) == 0;
}

core::expected<Footer, TGAError> parseFooter(u8* buffer, addr_size size) {
    if (size < sizeof(Footer)) {
        return core::unexpected(TGAError::InvalidFileFormat);
    }

    Footer* footerPtr = reinterpret_cast<Footer*>(buffer + (size - sizeof(Footer)));
    if (!hasSignature(footerPtr->signature)) {
        return core::unexpected(TGAError::OldFormat);
    }

    Footer footer = *footerPtr;
    return footer;
}

} // namespace TGA

#define FILE_PATH "shuttle.tga"

int main() {
    coreInit(core::LogLevel::L_DEBUG);

    {
        addr_size fileSize;
        u8* buffer;
        {
            core::FileStat stat;
            core::fileStat(ASSETS_DIRECTORY "/" FILE_PATH, stat);
            fileSize = stat.size;
            buffer = reinterpret_cast<u8*>(
                core::getAllocator(core::DEFAULT_ALLOCATOR_ID).alloc(fileSize, sizeof(u8))
            );
        }
        defer { core::getAllocator(core::DEFAULT_ALLOCATOR_ID).free(buffer, fileSize, sizeof(u8)); };

        auto mem = core::Memory(buffer, fileSize);
        core::Expect(core::fileReadEntire(ASSETS_DIRECTORY "/" FILE_PATH, mem));

        auto footerRes = TGA::parseFooter(mem.data(), mem.len());
        if (footerRes.hasValue()) {
            logInfo("File is in the new format: {}", footerRes.value().signature);
        }
        else if (footerRes.hasErr() && footerRes.err() == TGA::TGAError::OldFormat) {
            logInfo("File is in the old format.");
        }
        else {
            AssertFmt(false, "Failed to parse file err_code: {}", footerRes.err());
        }
    }

    coreShutdown();
    return 0;
}
