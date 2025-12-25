#include "surface.h"
#include "tga_files.h"

void Surface::free() {
    actx->free(data, addr_size(size()), sizeof(u8));
}

namespace {

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

} // namespace

core::expected<Surface, Error> createSurfaceFromTgaFile(const TGA::TGAFile& tgaFile, core::AllocatorContext& actx) {
    using namespace TGA;

    if (!tgaFile.isValid()) {
        return core::unexpected(Error::FailedToCreateSurface);
    }

    const Header* header;
    if (auto res = tgaFile.header(header); res.hasErr()) {
        return core::unexpected(Error::FailedToCreateSurface);
    }

    i32 height = header->height();
    i32 width = header->width();
    i32 pixelDepthInBits = header->pixelDepth();
    i32 bytesPerPixel = i32(f32(pixelDepthInBits) / f32(core::BYTE_SIZE) + 0.5f);
    i32 pitch = bytesPerPixel * width;
    i32 alphaChannelSize = header->alphaBits();

    addr_size imageSize = addr_size(pitch) * addr_size(height);
    u8* data = reinterpret_cast<u8*>(actx.alloc(imageSize, sizeof(u8)));

    PixelFormat pixelFormat = PixelFormat::Unknown;

    switch (header->imageType) {
        case 2:
            // True Color Image
            pixelFormat = pickPixelFormatForTrueColorImage(bytesPerPixel, alphaChannelSize);
            break;

        default:
            logErr("Unsupported tga image type: {}", i32(header->imageType));
            return core::unexpected(Error::FailedToCreateSurface);
    }

    if (pixelFormat == PixelFormat::Unknown) {
        logErr("pixel format unknown");
        return core::unexpected(Error::FailedToCreateSurface);
    }

    // TODO2: Decode if run-length encoded (RLE)
    addr_size imageDataOff = addr_size(tgaFile.imageDataOff);
    core::memcopy(data, &tgaFile.memory[imageDataOff], imageSize);

    Surface surface = {
        .actx = &actx,
        .pixelFormat = pixelFormat,
        .width = width,
        .height = height,
        .pitch = pitch,
        .data = data
    };
    return surface;
}
