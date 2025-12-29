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

PixelFormat pickPixelFormatForTrueColorImage(i32 bytesPerPixel, i32 alphaChannelSize);

core::expected<TGAError> createTrueColorFile(const CreateFileFromSurfaceParams& params);

} // namespace

const char* errorToCstr(TGAError err) {
    switch (err)
    {
        case TGAError::FailedToOpenFile:      return "Failed to open file";
        case TGAError::FailedToStatFile:      return "Failed to stat file";
        case TGAError::FailedToReadFile:      return "Failed to read file";
        case TGAError::FailedToWriteFile:     return "Failed to write file";
        case TGAError::InvalidFileFormat:     return "Invalid file format";
        case TGAError::OldFormat:             return "Old format";
        case TGAError::ApplicationBug:        return "User code has a bug";
        case TGAError::InvalidArgument:       return "Invalid argument passed";
        case TGAError::UnsupportedImageType:  return "Unsupported image type";
        case TGAError::FailedToCreateSurface: return "Failed to create surface";

        case TGAError::Undefined: [[fallthrough]];
        case TGAError::SENTINEL: [[fallthrough]];
        default: return "unknown";
    }
}

core::expected<TGAError> TGAImage::header(const Header*& out) const {
    if (fileHeaderOff < 0) {
        return core::unexpected(TGAError::ApplicationBug);
    }

    out = reinterpret_cast<const Header*>(memory.data() + fileHeaderOff);
    return {};
}

core::expected<TGAError> TGAImage::footer(const Footer*& out) const {
    if (footerOff < 0) {
        return core::unexpected(TGAError::OldFormat);
    }

    out = reinterpret_cast<const Footer*>(memory.data() + footerOff);
    return {};
}

FileType TGAImage::fileType() const {
    return footerOff != -1 ? FileType::New : FileType::Original;
}

bool TGAImage::isValid() const {
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

void TGAImage::free() {
    if (memory.data()) {
        actx->free(memory.data(), memory.len(), sizeof(u8));
        memory = {};
    }
}

core::expected<TGAImage, TGAError> loadFile(const char* path, core::AllocatorContext& actx) {
    TGAImage tgaFile;
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

core::expected<Surface, TGAError> createSurfaceFromTgaFile(const TGA::TGAImage& tgaFile, core::AllocatorContext& actx) {
    using namespace TGA;

    if (!tgaFile.isValid()) {
        logErr("Tga file is invalid");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    const Header* header;
    if (auto res = tgaFile.header(header); res.hasErr()) {
        logErr("Failed to parse header");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    i32 height = header->height();
    i32 width = header->width();
    i32 pixelDepthInBits = header->pixelDepth();
    i32 bytesPerPixel = i32(f32(pixelDepthInBits) / f32(core::BYTE_SIZE) + 0.5f);
    i32 pitch = bytesPerPixel * width;
    i32 alphaChannelSize = header->alphaBits();

    addr_size imageSize = addr_size(pitch) * addr_size(height);
    if (imageSize == 0) {
        logErr("Image size is 0");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    u8* data = reinterpret_cast<u8*>(actx.alloc(imageSize, sizeof(u8)));

    PixelFormat pixelFormat = PixelFormat::Unknown;

    switch (header->imageType) {
        case 2:
            // True Color Image
            pixelFormat = pickPixelFormatForTrueColorImage(bytesPerPixel, alphaChannelSize);
            break;

        // TODO2: [Support] Do I care for any other image type?
        // TODO2: [Support] Decode if run-length encoded (RLE).

        default:
            logErr("Unsupported tga image type: {}", i32(header->imageType));
            return core::unexpected(TGAError::FailedToCreateSurface);
    }

    if (pixelFormat == PixelFormat::Unknown) {
        logErr("pixel format unknown");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    addr_size imageDataOff = addr_size(tgaFile.imageDataOff);
    core::memcopy(data, &tgaFile.memory[imageDataOff], imageSize);

    Origin origin = Origin::Undefined;
    switch (header->origin()) {
        case 0b00: origin = Origin::BottomLeft;  break;
        case 0b01: origin = Origin::BottomRight; break;
        case 0b10: origin = Origin::TopLeft;     break;
        case 0b11: origin = Origin::TopRight;    break;
        default:   origin = Origin::Undefined;   break;
    }

    Surface surface = Surface();
    surface.actx = &actx;
    surface.origin = origin;
    surface.pixelFormat = pixelFormat;
    surface.width = width;
    surface.height = height;
    surface.pitch = pitch;
    surface.data = data;
    return surface;
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
        case TGAError::OldFormat:             return false;

        case TGAError::FailedToOpenFile:      return true;
        case TGAError::FailedToStatFile:      return true;
        case TGAError::FailedToReadFile:      return true;
        case TGAError::FailedToWriteFile:     return true;
        case TGAError::InvalidFileFormat:     return true;
        case TGAError::ApplicationBug:        return true;
        case TGAError::InvalidArgument:       return true;
        case TGAError::UnsupportedImageType:  return true;
        case TGAError::FailedToCreateSurface: return true;

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

PixelFormat pickPixelFormatForTrueColorImage(i32 bytesPerPixel, i32 alphaChannelSize) {
    PixelFormat pixelFormat = PixelFormat::Unknown;

    if (bytesPerPixel == 3) {
        if (alphaChannelSize != 0) {
            goto error;
        }

        pixelFormat = PixelFormat::BGR888;
    }
    else if (bytesPerPixel == 4) {
        if (alphaChannelSize == 8) {
            pixelFormat = PixelFormat::BGRA8888;
        }
        else if (alphaChannelSize == 0) {
            pixelFormat = PixelFormat::BGRX8888; // 8 padding bits, no alpha channel data
        }
        else {
            goto error;
        }
    }
    else if (bytesPerPixel == 2) {
        if (alphaChannelSize == 1) {
            pixelFormat = PixelFormat::BGRA5551;
        }
        else if (alphaChannelSize == 0) {
            pixelFormat = PixelFormat::BGR555;
        }
        else {
            goto error;
        }
    }
    else {
        goto error;
    }

    return pixelFormat;
error:
    logErr("bytesPerPixel = {}, but alphaChannelSize = {}", bytesPerPixel, alphaChannelSize);
    return PixelFormat::Unknown;
}

core::expected<TGAError> createTrueColorFile(const CreateFileFromSurfaceParams& params) {
    auto openRes = core::fileOpen(params.path,
        core::OpenMode::Read | core::OpenMode::Write | core::OpenMode::Truncate | core::OpenMode::Create);
    if (openRes.hasErr()) {
        logErr_PltErrorCode(openRes.err());
        return core::unexpected(TGAError::FailedToOpenFile);
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
