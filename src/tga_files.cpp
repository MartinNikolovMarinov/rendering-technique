#include "tga_files.h"
#include "log_utils.h"
#include "surface.h"

#define TGA_IS_ERR_FATAL(x) if (x.hasErr() && isFatalError(x.err())) return core::unexpected(x.err());

namespace TGA
{

namespace {

constexpr auto TRUE_VISION_SIGNATURE = "TRUEVISION-XFILE."_sv;

struct FormatMapping {
    u8 imageType;
    u8 bitsPerPixel;
    u8 alphaChannelSize;
    PixelFormat format;
};
constexpr FormatMapping formatMappingTable[] = {
    // True color (image type 2)
    { .imageType = 2, .bitsPerPixel = 16, .alphaChannelSize = 0, .format = PixelFormat::BGRX5551 },
    { .imageType = 2, .bitsPerPixel = 16, .alphaChannelSize = 1, .format = PixelFormat::BGRA5551 },
    { .imageType = 2, .bitsPerPixel = 24, .alphaChannelSize = 0, .format = PixelFormat::BGR888 },
    { .imageType = 2, .bitsPerPixel = 32, .alphaChannelSize = 8, .format = PixelFormat::BGRA8888 },
    { .imageType = 2, .bitsPerPixel = 32, .alphaChannelSize = 0, .format = PixelFormat::BGRX8888 },

    // Grayscale (image type 3)
    { .imageType = 3, .bitsPerPixel = 8, .alphaChannelSize = 0, .format = PixelFormat::GRAY8 },
    { .imageType = 3, .bitsPerPixel = 16, .alphaChannelSize = 8, .format = PixelFormat::GRAYA88 },
};

constexpr bool isFatalError(TGAError err);

constexpr bool hasSignature(const char signature[18]);
constexpr core::expected<addr_off, TGAError> parseFooterOffset(u8* begin, u8* end);

PixelFormat pickPixelFormat(u8 imageType, u8 bitsPerPixel, u8 alphaChannelSize);

core::expected<TGAError> createImageFile(const CreateFileFromSurfaceParams& params, const FormatMapping& mapping);

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

void TGAImage::free() {
    if (memory.data()) {
        actx->free(memory.data(), memory.len(), sizeof(u8));
        memory = {};
    }

    actx = nullptr;

    imageIdOff = -1;
    colorMapDataOff = -1;
    imageDataOff = -1;
    developerAreaOff = -1;
    extAreaOff = -1;
    footerOff = -1;
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

core::expected<i32, TGAError> TGAImage::imageType() const {
    const TGA::Header* h = nullptr;
    auto res = header(h);
    if (res.hasErr()) return core::unexpected(res.err());
    return h->imageType;
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

core::expected<TGAImage, TGAError> loadFile(const char* path, core::AllocatorContext& actx) {
    TGAImage tgaImage;
    tgaImage.actx = &actx;

    // Stat the file
    core::FileStat stat;
    {
        if (auto res = core::fileStat(path, stat); res.hasErr()) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToStatFile);
        }
        tgaImage.memory.length = stat.size;
        tgaImage.memory.ptr = reinterpret_cast<u8*>(
            tgaImage.actx->alloc(tgaImage.memory.length, sizeof(u8))
        );
    }

    // Read the entire file
    auto& memory = tgaImage.memory;
    {
        if (auto res = core::fileReadEntire(path, memory); res.hasErr()) {
            logErr_PltErrorCode(res.err());
            return core::unexpected(TGAError::FailedToReadFile);
        }
    }

    // Parse the footer
    auto footerOffsetRes = parseFooterOffset(memory.data(), memory.end());
    TGA_IS_ERR_FATAL(footerOffsetRes);
    tgaImage.footerOff = footerOffsetRes.hasValue() ? footerOffsetRes.value() : -1;

    if (tgaImage.footerOff > 0) {
        const Footer* f = nullptr;
        auto footerRes = tgaImage.footer(f);
        TGA_IS_ERR_FATAL(footerRes);
        tgaImage.developerAreaOff = f->developerDirectoryOffset;
        tgaImage.extAreaOff = f->extensionAreaOffset;
    }

    // Parse the header
    const Header* h = nullptr;
    if (auto res = tgaImage.header(h); res.hasErr()) {
        return core::unexpected(res.err());
    }

    // Parse the image/color map data area
    {
        addr_off curr = tgaImage.imageColorMapDataAreaOff;

        if (h->idLength > 0) {
            tgaImage.imageIdOff = curr;
            curr += h->idLength;
        }
        else {
            tgaImage.imageIdOff = -1;
        }

        if (h->colorMapType == 1) {
            tgaImage.colorMapDataOff = curr;
            [[maybe_unused]] i32 firstEntryIdx = h->colorMapFirstEntryIdx();
            i32 colorMapCount = h->colorMapLength();
            i32 colorMapEntrySize = h->colorMapEntrySize();
            curr += colorMapCount * colorMapEntrySize;
        }
        else {
            tgaImage.colorMapDataOff = -1;
        }

        tgaImage.imageDataOff = curr;
    }

    if (!tgaImage.isValid()) {
        return core::unexpected(TGAError::InvalidFileFormat);
    }

    return tgaImage;
}

core::expected<Surface, TGAError> createSurfaceFromTgaImage(const TGA::TGAImage& tgaImage, core::AllocatorContext& actx) {
    using namespace TGA;

    if (!tgaImage.isValid()) {
        logErr("Tga file is invalid");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    const Header* header;
    if (auto res = tgaImage.header(header); res.hasErr()) {
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
        case 2: // True Color Image
            pixelFormat = pickPixelFormat(2, u8(pixelDepthInBits), u8(alphaChannelSize));
            break;
        case 3: // Gray Scale Image
            pixelFormat = pickPixelFormat(3, u8(pixelDepthInBits), u8(alphaChannelSize));
            break;

        // TODO2: [TGA Support] Do I care about color mapped images?
        // TODO2: [TGA Support] Decode if run-length encoded (RLE).

        default:
            logErr("Unsupported tga image type: {}", i32(header->imageType));
            return core::unexpected(TGAError::FailedToCreateSurface);
    }

    if (pixelFormat == PixelFormat::Unknown) {
        logErr("pixel format unknown/unsupported");
        return core::unexpected(TGAError::FailedToCreateSurface);
    }

    addr_size imageDataOff = addr_size(tgaImage.imageDataOff);
    core::memcopy(data, &tgaImage.memory[imageDataOff], imageSize);

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

    PixelFormat pixelFormat = params.surface.pixelFormat;
    const FormatMapping* mapping = nullptr;
    for (addr_size i = 0; i < CORE_C_ARRLEN(formatMappingTable); i++) {
        auto& m = formatMappingTable[i];
        if (m.imageType == params.imageType && m.format == pixelFormat) {
            mapping = &m;
            break;
        }
    }
    if (mapping == nullptr) {
        logErr("Unsupported pixel format {} for image type {}", pixelFormatToCstr(pixelFormat) , params.imageType);
        return core::unexpected(TGAError::UnsupportedImageType);
    }

    auto ret = createImageFile(params, *mapping);
    return ret;
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

PixelFormat pickPixelFormat(u8 imageType, u8 bitsPerPixel, u8 alphaChannelSize) {
    PixelFormat ret = PixelFormat::Unknown;

    for (addr_size i = 0; i < CORE_C_ARRLEN(formatMappingTable); i++) {
        auto& m = formatMappingTable[i];
        if (m.imageType == imageType &&
            m.bitsPerPixel == bitsPerPixel &&
            m.alphaChannelSize == alphaChannelSize
        ) {
            ret = m.format;
            break;
        }
    }

    return ret;
}

core::expected<TGAError> createImageFile(const CreateFileFromSurfaceParams& params, const FormatMapping& mapping) {
    const char* path = params.path;
    const Surface& surface = params.surface;
    FileType fileType = params.fileType;

    auto openRes = core::fileOpen(path,
        core::OpenMode::Read | core::OpenMode::Write | core::OpenMode::Truncate | core::OpenMode::Create);
    if (openRes.hasErr()) {
        logErr_PltErrorCode(openRes.err());
        return core::unexpected(TGAError::FailedToOpenFile);
    }

    core::FileDesc file = std::move(openRes.value());
    defer { core::fileClose(file); };

    Header header = {};

    header.imageType = TGAByte(mapping.imageType);
    header.setWidth(u16(surface.width));
    header.setHeight(u16(surface.height));
    header.setPixelDepth(mapping.bitsPerPixel);
    header.setAlphaBits(mapping.alphaChannelSize);

    // Set image origin
    switch (surface.origin) {
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
    if (auto res = core::fileWrite(file, surface.data, addr_size(surface.size())); res.hasErr() || res.value() != addr_size(surface.size())) {
        logErr_PltErrorCode(res.err());
        return core::unexpected(TGAError::FailedToWriteFile);
    }

    // Write the footer if file type is new
    if (fileType == FileType::New) {
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
