#include "surface.h"
#include "tga_files.h"

core::expected<Surface, Error> createSurfaceFromTgaFile(TGA::TGAFile& tgaFile) {
    using namespace TGA;

    Header* header;
    if (auto res = tgaFile.header(header); res.hasErr()) {
        return core::unexpected(Error::FailedToCreateSurface);
    }

    i32 height = i32(header->imageSpecification[4]) | i32(header->imageSpecification[5]) << core::BYTE_SIZE;
    i32 width = i32(header->imageSpecification[6]) | i32(header->imageSpecification[7]) << core::BYTE_SIZE;

    Surface surface = {
        .pixelFormat = PixelFormat::Unknown,
        .width = width,
        .height = height,
        .pitch = 0,
        .data = nullptr // TODO: Who becomes the owner of the data here ?
    };
    return surface;
}
