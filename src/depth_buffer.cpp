#include "depth_buffer.h"

#include "surface.h"

void DepthBuffer::free() {
    if (isOwner() && data) {
        actx->free(data, addr_size(size()), sizeof(f32));
    }

    actx = nullptr;
    width = 0;
    height = 0;
    data = nullptr;
}

DepthBuffer createDepthBuffer(i32 width, i32 height, core::AllocatorContext& actx) {
    Assert(width > 0);
    Assert(height > 0);

    DepthBuffer ret = {
        .actx = &actx,
        .width = width,
        .height = height,
        .data = reinterpret_cast<f32*>(actx.zeroAlloc(addr_size(width * height), sizeof(f32)))
    };
    return ret;
}

void depthBufferToGrayscaleSurface(const DepthBuffer& depthBuffer, Surface& output) {
    Assert(depthBuffer.data != nullptr);
    Assert(output.pixelFormat == PixelFormat::GRAY8, "depth visualization requires GRAY8 output");
    Assert(output.width == depthBuffer.width);
    Assert(output.height == depthBuffer.height);

    for (i32 y = 0; y < output.height; y++) {
        for (i32 x = 0; x < output.width; x++) {
            f32 d = depthBuffer.at(x, y);
            f32 t = core::clamp(d, 0.0f, 1.0f);
            u8 gray = u8(t * 255.0f);
            i32 idx = y * output.pitch + x * output.bpp();
            output.data[idx] = gray;
        }
    }
}
