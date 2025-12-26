#include "tga_files.h"
#include "log_utils.h"
#include "surface.h"

#define TGA_IS_ERR_FATAL(x) if (x.hasErr() && isFatalError(x.err())) return core::unexpected(x.err());

namespace TGA
{

namespace {

constexpr auto TRUE_VISION_SIGNATURE = "TRUEVISION-XFILE."_sv;

constexpr bool isFatalError(TGAError err);

constexpr bool hasSignature(const char signature[18]);
constexpr core::expected<addr_off, TGAError> parseFooterOffset(u8* begin, u8* end);

core::expected<TGAError> createTrueColorFile(const CreateFileFromSurfaceParams& params);

} // namespace

const char* errorToCstr(TGAError err) {
    switch (err)
    {
        case TGAError::FailedToStatFile:     return "Failed to stat file";
        case TGAError::FailedToReadFile:     return "Failed to read file";
        case TGAError::FailedToWriteFile:    return "Failed to write file";
        case TGAError::InvalidFileFormat:    return "Invalid file format";
        case TGAError::OldFormat:            return "Old format";
        case TGAError::ApplicationBug:       return "User code has a bug";
        case TGAError::InvalidArgument:      return "Invalid argument passed";
        case TGAError::UnsupportedImageType: return "Unsupported image type";

        case TGAError::Undefined: [[fallthrough]];
        case TGAError::SENTINEL: [[fallthrough]];
        default: return "unknown";
    }
}

core::expected<TGAFile, TGAError> loadFile(const char* path, core::AllocatorContext& actx) {
    TGAFile tgaFile;
    tgaFile.actx = &actx;

    // Stat the file
    core::FileStat stat;
    {
        if (auto res = core::fileStat(path, stat); res.hasErr()) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToStatFile);
        }
        tgaFile.memory.length = stat.size;
        tgaFile.memory.ptr = reinterpret_cast<u8*>(
            tgaFile.actx->alloc(tgaFile.memory.length, sizeof(u8))
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

    // Parse the footer
    auto footerOffsetRes = parseFooterOffset(memory.data(), memory.end());
    TGA_IS_ERR_FATAL(footerOffsetRes);
    tgaFile.footerOff = footerOffsetRes.hasValue() ? footerOffsetRes.value() : -1;

    if (tgaFile.footerOff > 0) {
        const Footer* f = nullptr;
        auto footerRes = tgaFile.footer(f);
        TGA_IS_ERR_FATAL(footerRes);
        tgaFile.developerAreaOff = f->developerDirectoryOffset;
        tgaFile.extAreaOff = f->extensionAreaOffset;
    }

    // Parse the header
    const Header* h = nullptr;
    if (auto res = tgaFile.header(h); res.hasErr()) {
        return core::unexpected(res.err());
    }

    // Parse the image/color map data area
    {
        addr_off curr = tgaFile.imageColorMapDataAreaOff;

        if (h->idLength > 0) {
            tgaFile.imageIdOff = curr;
            curr += h->idLength;
        }
        else {
            tgaFile.imageIdOff = -1;
        }

        if (h->colorMapType == 1) {
            tgaFile.colorMapDataOff = curr;
            [[maybe_unused]] i32 firstEntryIdx = h->colorMapFirstEntryIdx();
            i32 colorMapCount = h->colorMapLength();
            i32 colorMapEntrySize = h->colorMapEntrySize();
            curr += colorMapCount * colorMapEntrySize;
        }
        else {
            tgaFile.colorMapDataOff = -1;
        }

        tgaFile.imageDataOff = curr;
    }

    if (!tgaFile.isValid()) {
        return core::unexpected(TGAError::InvalidFileFormat);
    }

    return tgaFile;
}

core::expected<TGAError> TGAFile::header(const Header*& out) const {
    if (fileHeaderOff < 0) {
        return core::unexpected(TGAError::ApplicationBug);
    }

    out = reinterpret_cast<const Header*>(memory.data() + fileHeaderOff);
    return {};
}

core::expected<TGAError> TGAFile::footer(const Footer*& out) const {
    if (footerOff < 0) {
        return core::unexpected(TGAError::OldFormat);
    }

    out = reinterpret_cast<const Footer*>(memory.data() + footerOff);
    return {};
}

FileType TGAFile::fileType() const {
    return footerOff != -1 ? FileType::New : FileType::Original;
}

bool TGAFile::isValid() const {
    bool ok = imageDataOff > 0 && memory.data() != nullptr && memory.length > 0;
    if (fileType() == TGA::FileType::New) {
        const TGA::Footer* f = nullptr;
        if (auto res = footer(f); res.hasErr()) {
            ok = false;
        }
        else {
            ok &= f->developerDirectoryOffset == developerAreaOff &&
                  f->extensionAreaOffset == extAreaOff &&
                  hasSignature(f->signature);
        }
    }
    return ok;
}

void TGAFile::free() {
    if (memory.data()) {
        actx->free(memory.data(), memory.len(), sizeof(u8));
        memory = {};
    }
}

core::expected<TGAError> createFileFromSurface(const CreateFileFromSurfaceParams& params) {
    if (params.surface.size() == 0) {
        logErr("Surface size is 0");
        return core::unexpected(TGAError::InvalidArgument);
    }

    bool isValidFileType = (params.fileType == FileType::New) || (params.fileType == FileType::Original);
    if (!isValidFileType) {
        logErr("Invalid file type = {}", params.fileType);
        return core::unexpected(TGAError::InvalidArgument);
    }

    switch (params.imageType) {
        case 2:
            return createTrueColorFile(params);

        // TODO2: [Support] Do I cae for any other image type?
        // TODO2: [Support] Run-length encoding (RLE).

        default:
            logErr("Unsupported image type = {}", params.imageType);
            return core::unexpected(TGAError::UnsupportedImageType);
    }

    return {};
}

namespace
{

constexpr bool isFatalError(TGAError err) {
    switch (err)
    {
        case TGAError::OldFormat:            return false;

        case TGAError::FailedToStatFile:     return true;
        case TGAError::FailedToReadFile:     return true;
        case TGAError::FailedToWriteFile:    return true;
        case TGAError::InvalidFileFormat:    return true;
        case TGAError::ApplicationBug:       return true;
        case TGAError::InvalidArgument:      return true;
        case TGAError::UnsupportedImageType: return true;

        case TGAError::Undefined: [[fallthrough]];
        case TGAError::SENTINEL: [[fallthrough]];
        default: return true;
    }
}

constexpr bool hasSignature(const char signature[18]) {
    return core::memcmp(signature, 17, TRUE_VISION_SIGNATURE.data(), TRUE_VISION_SIGNATURE.len()) == 0;
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

core::expected<TGAError> createTrueColorFile(const CreateFileFromSurfaceParams& params) {
    auto openRes = core::fileOpen(params.path,
        core::OpenMode::Read | core::OpenMode::Write | core::OpenMode::Truncate | core::OpenMode::Create);
    if (openRes.hasErr()) {
        logErr_PltErrorCode(openRes.err());
        return core::unexpected(TGAError::InvalidArgument);
    }

    core::FileDesc file = std::move(openRes.value());

    Header header = {};

    header.imageType = TGAByte(params.imageType);
    header.setWidth(u16(params.surface.width));
    header.setHeight(u16(params.surface.height));
    header.setPixelDepth(u8(pixelFormatBytesPerPixel(params.surface.pixelFormat) * core::BYTE_SIZE));
    header.setAlphaBits(u8(pixelFormatAlphaBits(params.surface.pixelFormat)));

    // Set image origin
    switch (params.surface.origin) {
        case Origin::BottomLeft:
            header.setOrigin(0b00);
            break;
        case Origin::BottomRight:
            header.setOrigin(0b01);
            break;
        case Origin::TopLeft:
            header.setOrigin(0b10);
            break;
        case Origin::TopRight:
            header.setOrigin(0b11);
            break;

        case Origin::Undefined: [[fallthrough]];
        case Origin::Center:    [[fallthrough]];
        case Origin::SENTINEL:
            Assert(false, "unsupported surface origin");
            return core::unexpected(TGAError::InvalidArgument);
    }

    // Write the header
    if (auto res = core::fileWrite(file, &header, sizeof(Header)); res.hasErr() || res.value() != sizeof(Header)) {
        logErr_PltErrorCode(res.err());
        return core::unexpected(TGAError::FailedToWriteFile);
    }

    // Write the content
    if (auto res = core::fileWrite(file, params.surface.data, addr_size(params.surface.size())); res.hasErr() || res.value() != addr_size(params.surface.size())) {
        logErr_PltErrorCode(res.err());
        return core::unexpected(TGAError::FailedToWriteFile);
    }

    // Write the footer if file type is new
    if (params.fileType == FileType::New) {
        Footer footer = {};
        core::memcopy(footer.signature, TRUE_VISION_SIGNATURE.data(), TRUE_VISION_SIGNATURE.len());
        if (auto res = core::fileWrite(file, &footer, sizeof(Footer)); res.hasErr() || res.value() != sizeof(Footer)) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToWriteFile);
        }
    }

    return {};
}

} // namespace

} // namespace TGA
