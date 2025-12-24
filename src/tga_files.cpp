#include "tga_files.h"
#include "log_utils.h"

#define TGA_IS_ERR_FATAL(x) if (x.hasErr() && isFatalError(x.err())) return core::unexpected(x.err());

namespace TGA
{

namespace {

constexpr bool isFatalError(TGAError err);

constexpr bool hasSignature(char signature[16]);
constexpr core::expected<addr_off, TGAError> parseFooterOffset(u8* begin, u8* end);

} // namespace

const char* errorToCstr(TGAError err) {
    switch (err)
    {
        case TGAError::FailedToStatFile:  return "Failed to stat file";
        case TGAError::FailedToReadFile:  return "Failed to read file";
        case TGAError::InvalidFileFormat: return "Invalid file format";
        case TGAError::OldFormat:         return "Old format";
        case TGAError::ApplicationBug:    return "User code has a bug";

        case TGAError::Undefined: [[fallthrough]];
        case TGAError::SENTINEL: [[fallthrough]];
        default: return "unknown";
    }
}

core::expected<TGAFile, TGAError> loadFile(const char* path, const core::AllocatorContext& actx) {
    TGAFile tgaFile;
    tgaFile.actx = &actx;

    {
        // Just in case set them all to -1 initially
        tgaFile.fileHeaderOff = 0;
        tgaFile.imageColorDataOff = sizeof(Header);
        tgaFile.footerOff = -1;
        tgaFile.developerAreaOff = -1;
        tgaFile.extAreaOff = -1;
    }

    // Stat the file
    core::FileStat stat;
    {
        if (auto res = core::fileStat(path, stat); res.hasErr()) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToStatFile);
        }
        tgaFile.memory.length = stat.size;
        tgaFile.memory.ptr = reinterpret_cast<u8*>(
            actx.allocFn(actx.allocatorData, tgaFile.memory.length, sizeof(u8))
        );
    }

    // Read the entire file
    auto& memory = tgaFile.memory;
    {
        if (auto res = core::fileReadEntire(path, memory); res.hasErr()) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToReadFile);
        }
    }

    // Parse the footer offset
    {
        auto res = parseFooterOffset(memory.data(), memory.end());
        TGA_IS_ERR_FATAL(res);
        tgaFile.footerOff = res.hasValue() ? res.value() : -1;
    }

    return tgaFile;
}

core::expected<TGAError> TGAFile::header(Header*& out) {
    if (fileHeaderOff < 0) {
        return core::unexpected(TGAError::ApplicationBug);
    }

    out = reinterpret_cast<Header*>(memory.data() + fileHeaderOff);
    return {};
}

core::expected<TGAError> TGAFile::footer(Footer*& out) {
    if (footerOff < 0) {
        return core::unexpected(TGAError::OldFormat);
    }

    out = reinterpret_cast<Footer*>(memory.data() + footerOff);
    return {};
}

FileType TGAFile::fileType() {
    return footerOff != -1 ? FileType::New : FileType::Original;
}

void TGAFile::free() {
    if (memory.data()) {
        actx->freeFn(actx->allocatorData, memory.data(), memory.len(), sizeof(u8));
        memory = {};
    }
}

namespace
{

constexpr bool isFatalError(TGAError err) {
    switch (err)
    {
        case TGAError::FailedToStatFile:  return true;
        case TGAError::FailedToReadFile:  return true;
        case TGAError::InvalidFileFormat: return true;
        case TGAError::OldFormat:         return false;
        case TGAError::ApplicationBug:    return true;

        case TGAError::Undefined: [[fallthrough]];
        case TGAError::SENTINEL: [[fallthrough]];
        default: return true;
    }
}

constexpr bool hasSignature(char signature[16]) {
    return core::memcmp(signature, 16, "TRUEVISION-XFILE", 16) == 0;
}

constexpr core::expected<addr_off, TGAError> parseFooterOffset(u8* begin, u8* end) {
    constexpr addr_off footerSize = addr_off(sizeof(Footer));

    addr_off size = core::ptrDiff(end, begin);
    if (size < footerSize) {
        return core::unexpected(TGAError::InvalidFileFormat);
    }

    addr_off off = size - footerSize;
    Footer* footerPtr = reinterpret_cast<Footer*>(&begin[off]);
    if (!hasSignature(footerPtr->signature)) {
        return core::unexpected(TGAError::OldFormat);
    }

    return off;
}

} // namespace


} // namespace TGA
