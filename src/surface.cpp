#include "surface.h"
#include "tga_files.h"

void Surface::free() {
    actx->free(data, addr_size(size()), sizeof(u8));
}

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
    i32 bytesPerPixel = pixelDepthInBits / core::BYTE_SIZE;
    i32 pitch = bytesPerPixel * width;
    i32 alphaChannelSize = header->alphaBits();

    logInfo("Alpha channel size: {}", alphaChannelSize);

    addr_size imageSize = addr_size(pitch) * addr_size(height);
    u8* data = reinterpret_cast<u8*>(actx.alloc(imageSize, sizeof(u8)));

    // TODO2: Decode if run-length encoded (RLE)
    addr_size imageDataOff = addr_size(tgaFile.imageDataOff);
    core::memcopy(data, &tgaFile.memory[imageDataOff], imageSize);

    Surface surface = {
        .actx = &actx,
        .pixelFormat = PixelFormat::Unknown,
        .width = width,
        .height = height,
        .pitch = pitch,
        .data = data
    };
    return surface;
}
