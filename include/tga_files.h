#include "core_init.h"

namespace TGA
{

using TGALong = u32;
using TGAShort = u16;
using TGAByte = u8;

enum struct TGAError {
    Undefined,
    FailedToStatFile,
    FailedToReadFile,
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

enum struct FileType {
    Unknown,
    Original,
    New
};

struct TGAFile {
    const core::AllocatorContext* actx;

    core::Memory<u8> memory;
    addr_off fileHeaderOff;
    addr_off imageColorDataOff;
    addr_off footerOff;
    addr_off developerAreaOff;
    addr_off extAreaOff;

    core::expected<TGAError> header(Header*& out);
    core::expected<TGAError> imageColorMapData(ImageColorMapData*& out);
    core::expected<TGAError> footer(Footer*& out);
    core::expected<TGAError> developerArea(DeveloperArea*& out);
    core::expected<TGAError> extArea(ExtensionArea*& out);

    FileType fileType();

    void free();
};

const char* errorToCstr(TGAError err);

core::expected<TGAFile, TGAError> loadTgaFile(const char* path, const core::AllocatorContext& actx = DEF_ALLOC);

} // namespace TGA
