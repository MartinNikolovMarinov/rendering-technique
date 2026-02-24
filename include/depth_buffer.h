#pragma once

#include "core_init.h" // IWYU pragma: keep

#include "surface.h"

struct DepthBuffer {
    core::AllocatorContext* actx = nullptr;
    i32 width = 0;
    i32 height = 0;
    f32* data = nullptr;

    constexpr i32 size() const { return width * height; }
    constexpr bool isOwner() const { return actx != nullptr; }

    constexpr f32 at(i32 x, i32 y) const {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);

        return data[y * width + x];
    }

    constexpr f32& at(i32 x, i32 y) {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);

        return data[y * width + x];
    }

    constexpr void clear(f32 depth) {
        core::memset(data, depth, addr_size(size()));
    }

    void free();
};

DepthBuffer createDepthBuffer(i32 width, i32 height, core::AllocatorContext& actx);
void depthBufferToGrayscaleSurface(const DepthBuffer& depthBuffer, Surface& output);
