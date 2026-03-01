#include "surface_renderer.h"

#include "color.h"
#include "face.h"
#include "model.h"
#include "face.h"
#include "surface.h"
#include "depth_buffer.h"

namespace {

using SetPixelFn = void (*)(u8* data, i32 idx, Color color);

constexpr inline void setPixelRaw_BGRA8888(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_BGRX8888(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_BGR888(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_BGRA5551(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_BGRX5551(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_GRAY8(u8* data, i32 idx, Color color);
constexpr inline void setPixelRaw_GRAYA88(u8* data, i32 idx, Color color);

constexpr inline SetPixelFn pickSetPixelFunction(PixelFormat pixelFormat);

void fillPixelGuarded(Surface& surface, i32 x, i32 y, Color color);

} // namespace

void fillPixel(Surface& surface, i32 x, i32 y, Color color) {
    i32 idx = y * surface.pitch + x * surface.bpp();

    Assert(surface.data != nullptr, "surface data is null");
    Assert(y >= 0 && y < surface.height, "y out of bounds");
    Assert(x >= 0 && x < surface.width, "x out of bounds");
    Assert(idx + surface.bpp() <= surface.size(), "pixel write past end of surface");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
    setPixelFn(surface.data, idx, color);
}

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color) {
    Assert(surface.data != nullptr, "surface data is null");
    Assert(surface.bpp() > 0, "invalid bytes-per-pixel");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);

    // Bresenham line drawing algorithm using integer calculations.

    bool transpose = core::absGeneric(ax - bx) < core::absGeneric(ay - by);
    if (transpose) {
        core::swap(ax, ay);
        core::swap(bx, by);
    }

    bool flipLeftToRight = ax > bx;
    if (flipLeftToRight) {
        core::swap(ax, bx);
        core::swap(ay, by);
    }

    i32 y = ay;
    i32 ierror = 0;
    i32 dty2 = core::absGeneric(by - ay) * 2;
    i32 dtx2 = (bx - ax) * 2;
    i32 ydir = by > ay ? 1 : -1;
    for (i32 x = ax; x <= bx; x++) {
        i32 idx = (transpose)
            ? x * surface.pitch + y * surface.bpp()
            : y * surface.pitch + x * surface.bpp();

        // TODO: [PERFORMANCE] Is there a branchless way to do this upfront?
        if (0 < idx && idx <= surface.size()) {
            setPixelFn(surface.data, idx, color);
        }

        ierror += dty2;
        if (ierror > bx - ax) {
            y += ydir;
            ierror -= dtx2;
        }
    }
}

void fillRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height) {
    Assert(surface.data != nullptr, "surface data is null");
    Assert(width > 0 && height > 0, "rect has non-positive size");
    Assert(x >= 0 && y >= 0, "rect origin out of bounds");
    Assert(y + height <= surface.height, "rect extends past surface height");
    Assert(x + width  <= surface.width,  "rect extends past surface width");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
    for (i32 row = y; row < y + height; row++) {
        for (i32 col = x; col < x + width; col++) {
            i32 idx = row * surface.pitch + col * surface.bpp();
            setPixelFn(surface.data, idx, color);
        }
    }
}

void strokeRect(Surface& surface, i32 x, i32 y, Color color, i32 width, i32 height) {
    i32 endX = x + width - 1;
    i32 endY = y + height - 1;
    fillLine(surface, x, y, endX, y, color);
    fillLine(surface, endX, y, endX, endY, color);
    fillLine(surface, endX, endY, x, endY, color);
    fillLine(surface, x, endY, x, y, color);
}

void strokeBBox(Surface& surface, const core::Bbox2D<i32>& bbox, Color color) {
    i32 x = bbox.min.x();
    i32 y = bbox.min.y();
    i32 endX = bbox.max.x();
    i32 endY = bbox.max.y();
    fillLine(surface, x, y, endX, y, color);
    fillLine(surface, endX, y, endX, endY, color);
    fillLine(surface, endX, endY, x, endY, color);
    fillLine(surface, x, endY, x, y, color);
}

//======================================================================================================================
// Triangle Rendering
//======================================================================================================================

namespace {

struct ProjectedVertex {
    core::vec2i p;
    f32 z;
};

void fillTriangleBarycentric(
    Surface& surface,
    const DepthBuffer* depthBuffer,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 holeInsetRatio
) {
    if (depthBuffer) {
        Assert(surface.width == depthBuffer->width);
        Assert(surface.height == depthBuffer->height);
    }

    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a.xy(), b.xy(), c.xy());
    bbox.clampTo(0, surface.width - 1, 0, surface.height - 1);

    f32 totalArea = core::calcTriangleAreaF32(a.xy(), b.xy(), c.xy());
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.x(), b.y(), c.x(), c.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.x(), c.y(), a.x(), a.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.x(), a.y(), b.x(), b.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            // Check point against depth buffer:
            if (depthBuffer) {
                f32 z = alpha * f32(a.z()) + beta * f32(b.z()) + gamma * f32(c.z());
                f32 depth = depthBuffer->at(x, y);
                if (z < depth) {
                    continue;
                }
            }

            // If a hole ratio is provided don't render pixels inside the hole:
            if (holeInsetRatio > 0.0f) {
                constexpr f32 barycentricComponentCount = 3.0f;
                f32 t = holeInsetRatio / barycentricComponentCount;
                if (alpha >= t && beta >= t && gamma >= t) {
                    continue;
                }
            }

            Color blendedColor = {
                .rgba {
                    .r = u8(alpha * f32(colorA.r()) + beta * f32(colorB.r()) + gamma * f32(colorC.r())),
                    .g = u8(alpha * f32(colorA.g()) + beta * f32(colorB.g()) + gamma * f32(colorC.g())),
                    .b = u8(alpha * f32(colorA.b()) + beta * f32(colorB.b()) + gamma * f32(colorC.b())),
                    .a = u8(alpha * f32(colorA.a()) + beta * f32(colorB.a()) + gamma * f32(colorC.a()))
                }
            };

            fillPixel(surface, x, y, blendedColor);
        }
    }
}

void fillDepthBuffer(
    DepthBuffer& depthBuffer,
    const ProjectedVertex& a, const ProjectedVertex& b, const ProjectedVertex& c
) {
    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a.p, b.p, c.p);
    bbox.clampTo(0, depthBuffer.width - 1, 0, depthBuffer.height - 1);

    f32 totalArea = core::calcTriangleAreaF32(a.p, b.p, c.p);
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.p.x(), b.p.y(), c.p.x(), c.p.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.p.x(), c.p.y(), a.p.x(), a.p.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.p.x(), a.p.y(), b.p.x(), b.p.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            f32 z = alpha * a.z + beta * b.z + gamma * c.z;
            f32 depth = depthBuffer.at(x, y);
            if (z <= depth) {
                continue;
            }
            depthBuffer.at(x, y) = z;
        }
    }
}

void fillTriangle(
    Surface& surface,
    const DepthBuffer* depthBuffer,
    const ProjectedVertex& a, const ProjectedVertex& b, const ProjectedVertex& c,
    const Color& colorA, const Color& colorB, const Color& colorC
) {
    if (depthBuffer) {
        Assert(surface.width == depthBuffer->width);
        Assert(surface.height == depthBuffer->height);
    }

    core::Bbox2D<i32> bbox = core::calcTriangleBBox(a.p, b.p, c.p);
    bbox.clampTo(0, surface.width - 1, 0, surface.height - 1);

    f32 totalArea = core::calcTriangleAreaF32(a.p, b.p, c.p);
    if (core::absGeneric(totalArea) < 1) {
        return;
    }

    for (i32 x = bbox.min.x(); x <= bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y <= bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.p.x(), b.p.y(), c.p.x(), c.p.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.p.x(), c.p.y(), a.p.x(), a.p.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.p.x(), a.p.y(), b.p.x(), b.p.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                continue;
            }

            if (depthBuffer) {
                f32 z = alpha * a.z + beta * b.z + gamma * c.z;
                f32 depth = depthBuffer->at(x, y);
                if (z < depth) {
                    continue;
                }
            }

            Color blendedColor = {
                .rgba {
                    .r = u8(alpha * f32(colorA.r()) + beta * f32(colorB.r()) + gamma * f32(colorC.r())),
                    .g = u8(alpha * f32(colorA.g()) + beta * f32(colorB.g()) + gamma * f32(colorC.g())),
                    .b = u8(alpha * f32(colorA.b()) + beta * f32(colorB.b()) + gamma * f32(colorC.b())),
                    .a = u8(alpha * f32(colorA.a()) + beta * f32(colorB.a()) + gamma * f32(colorC.a()))
                }
            };

            fillPixel(surface, x, y, blendedColor);
        }
    }
}

} // namespace

void strokeTriangleFast(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& color
) {
    fillLine(surface, a.x(), a.y(), b.x(), b.y(), color);
    fillLine(surface, b.x(), b.y(), c.x(), c.y(), color);
    fillLine(surface, c.x(), c.y(), a.x(), a.y(), color);
}

void strokeTriangleInset(
    Surface& surface,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
) {
    f32 clampedRatio = core::core_max(0.0f, core::core_min(boarderRatio, 1.0f));
    fillTriangleBarycentric(surface, nullptr, a, b, c, colorA, colorB, colorC, clampedRatio);
}

void fillTriangle(
    Surface& surface,
    const core::vec3i& a, const core::vec3i& b, const core::vec3i& c,
    const Color& colorA, const Color& colorB, const Color& colorC
) {
    fillTriangleBarycentric(surface, nullptr, a, b, c, colorA, colorB, colorC, 0.0f);
}

//======================================================================================================================
// Statefull Rendering Section
//======================================================================================================================

namespace {

// FIXME: Replace with core functions.

constexpr inline ProjectedVertex orthogonalProjection(core::vec3f normVec, i32 width, i32 height) {
    i32 x = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
    i32 y = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
    f32 z = (normVec.z() + 1.0f) * 0.5f;

    ProjectedVertex ret = {};
    ret.p = core::v(x, y);
    ret.z = z;
    return ret;
}

inline core::vec3f rot(core::vec3f v, core::radians angle) {
    auto a = angle.value;
    core::mat3x3f Ry = core::mat<3, 3, f32>(
        core::v(core::cos(a), 0.f, core::sin(a)),
        core::v(0.f, 1.f, 0.f),
        core::v(-core::sin(a), 0.f, core::cos(a))
    );
    Ry = core::mtranspose(Ry);
    auto ret = Ry*v;
    return ret;
}

constexpr inline core::vec3f persp(core::vec3f v) {
    constexpr f32 c = 3.f;
    auto ret = v / (1.0f-v.z() / c);
    return ret;
}

}

struct RenderPassState {
    DepthBuffer* depthBuffer;
    core::Memory<Vertex4f> vertices;
    core::Memory<Face3i> faces;
};

struct Renderer {
    core::AllocatorContext* actx;
    i32 frameBufferWidth;
    i32 frameBufferHeight;
    bool wireframe;
    Surface* output;

    RenderPassState renderPass;
};

RendererHandle rendererInit(core::AllocatorContext& actx) {
    Renderer* ret = reinterpret_cast<Renderer*>(actx.alloc(1, sizeof(Renderer)));
    *ret = {};
    ret->actx = &actx;
    return ret;
}

void rendererDestory(RendererHandle r) {
    if (r && r->actx) {
        // Destory logic below:

        // Finalize free the renderer itself and zero-out all fields:
        r->actx->free(r, 1, sizeof(Renderer));
        r = {};
    }
}

void rendererSetFrameBuffer(RendererHandle r, i32 width, i32 height) {
    r->frameBufferWidth = width;
    r->frameBufferHeight = height;
}

void rendererSetWireframe(RendererHandle r, bool wireframe) {
    r->wireframe = wireframe;
}

void rendererSetOutput(RendererHandle r, Surface& output) {
    r->output = &output;
}

void rendererBeginFrame(RendererHandle) {
}

void rendererClear(RendererHandle r, const Color& c) {
    fillRect(*r->output, 0, 0, c, r->frameBufferWidth, r->frameBufferHeight);
}

void rendererSetVertexBuffer(RendererHandle r, core::Memory<Vertex4f> vertices) {
    r->renderPass.vertices = vertices;
}

void rendererSetIndexBuffer(RendererHandle r, core::Memory<Face3i> indices) {
    r->renderPass.faces = indices;
}

void rendererCalculateDepthBuffer(RendererHandle r, DepthBuffer& depthBuffer) {
    r->renderPass.depthBuffer = &depthBuffer;

    bool wireframeMode = r->wireframe;

    // Calculate depth buffer:
    if (!wireframeMode) {
        auto& vertices = r->renderPass.vertices;
        auto& faces = r->renderPass.faces;

        i32 width = r->frameBufferWidth;
        i32 height = r->frameBufferHeight;

        for (addr_size i = 0; i < faces.len(); i++) {
            auto& f = faces[i];

            core::vec4f& v1 = vertices[f[0]];
            core::vec4f& v2 = vertices[f[1]];
            core::vec4f& v3 = vertices[f[2]];

            // TODO: This projections is hardcoded for testing:
            // ProjectedVertex a = orthogonalProjection(persp(rot(v1.xyz(), core::degToRad(30))), width, height);
            // ProjectedVertex b = orthogonalProjection(persp(rot(v2.xyz(), core::degToRad(30))), width, height);
            // ProjectedVertex c = orthogonalProjection(persp(rot(v3.xyz(), core::degToRad(30))), width, height);
            ProjectedVertex a = orthogonalProjection(v1.xyz(), width, height);
            ProjectedVertex b = orthogonalProjection(v2.xyz(), width, height);
            ProjectedVertex c = orthogonalProjection(v3.xyz(), width, height);

            fillDepthBuffer(depthBuffer, a, b, c);
        }
    }
}

void rendererEndFrame(RendererHandle r) {
    auto& surface = *r->output;
    bool wireframe = r->wireframe;
    auto& vertices = r->renderPass.vertices;
    auto& faces = r->renderPass.faces;

    Assert(surface.width>= r->frameBufferWidth);
    Assert(surface.height >= r->frameBufferHeight);
    i32 width = surface.width;
    i32 height = surface.height;

    for (addr_size i = 0; i < faces.len(); i++) {
        auto& f = faces[i];

        core::vec4f& v1 = vertices[f[0]];
        core::vec4f& v2 = vertices[f[1]];
        core::vec4f& v3 = vertices[f[2]];

        // TODO: This projections is hardcoded for testing:
        // ProjectedVertex a = orthogonalProjection(persp(rot(v1.xyz(), core::degToRad(30))), width, height);
        // ProjectedVertex b = orthogonalProjection(persp(rot(v2.xyz(), core::degToRad(30))), width, height);
        // ProjectedVertex c = orthogonalProjection(persp(rot(v3.xyz(), core::degToRad(30))), width, height);
        ProjectedVertex a = orthogonalProjection(v1.xyz(), width, height);
        ProjectedVertex b = orthogonalProjection(v2.xyz(), width, height);
        ProjectedVertex c = orthogonalProjection(v3.xyz(), width, height);

        if (wireframe) {
            strokeTriangleFast(surface, a.p, b.p, c.p, RED);
            fillPixelGuarded(surface, a.p.x(), a.p.y(), WHITE);
            fillPixelGuarded(surface, b.p.x(), b.p.y(), WHITE);
            fillPixelGuarded(surface, c.p.x(), c.p.y(), WHITE);
        }
        else {
            Assert(r->renderPass.depthBuffer != nullptr, "depth buffer is required for filled rendering");
            auto& depthBuffer = *r->renderPass.depthBuffer;
            Color color1 = randomColor();
            Color color2 = randomColor();
            Color color3 = randomColor();
            fillTriangle(surface, &depthBuffer, a, b, c, color1, color2, color3);
        }
    }
}

namespace {

constexpr inline void setPixelRaw_BGRA8888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
    data[idx + 3] = color.a();
}

constexpr inline void setPixelRaw_BGRX8888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
    data[idx + 3] = 0;
}

constexpr inline void setPixelRaw_BGR888(u8* data, i32 idx, Color color) {
    data[idx + 0] = color.b();
    data[idx + 1] = color.g();
    data[idx + 2] = color.r();
}

constexpr inline void setPixelRaw_BGRA5551(u8* data, i32 idx, Color color) {
    // Packed as: bits 0-4 blue, 5-9 green, 10-14 red, 15 alpha.
    u16 b = u16(color.b() >> 3);
    u16 g = u16(color.g() >> 3);
    u16 r = u16(color.r() >> 3);
    u16 a = u16(color.a() >> 7);
    u16 packed = u16(b | (g << 5) | (r << 10) | (a << 15));
    data[idx + 0] = u8(packed & 0xFF);
    data[idx + 1] = u8(packed >> 8);
}

constexpr inline void setPixelRaw_BGRX5551(u8* data, i32 idx, Color color) {
    // Packed as: bits 0-4 blue, 5-9 green, 10-14 red, bit 15 cleared.
    u16 b = u16(color.b() >> 3);
    u16 g = u16(color.g() >> 3);
    u16 r = u16(color.r() >> 3);
    u16 packed = u16(b | (g << 5) | (r << 10));
    data[idx + 0] = u8(packed & 0xFF);
    data[idx + 1] = u8(packed >> 8);
}

constexpr inline void setPixelRaw_GRAY8(u8* data, i32 idx, Color color) {
    // Use red channel as luminance source.
    data[idx + 0] = color.r();
}

constexpr inline void setPixelRaw_GRAYA88(u8* data, i32 idx, Color color) {
    // Use red channel as luminance source, alpha in second byte.
    data[idx + 0] = color.r();
    data[idx + 1] = color.a();
}

constexpr inline SetPixelFn pickSetPixelFunction(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::BGRA8888: return setPixelRaw_BGRA8888;
        case PixelFormat::BGRX8888: return setPixelRaw_BGRX8888;
        case PixelFormat::BGR888:   return setPixelRaw_BGR888;
        case PixelFormat::BGRA5551: return setPixelRaw_BGRA5551;
        case PixelFormat::BGRX5551: return setPixelRaw_BGRX5551;

        case PixelFormat::GRAY8:    return setPixelRaw_GRAY8;
        case PixelFormat::GRAYA88:  return setPixelRaw_GRAYA88;

        case PixelFormat::Unknown: [[fallthrough]];
        case PixelFormat::SENTINEL: [[fallthrough]];
        default:
            Assert(false, "invalid pixel format");
            return nullptr;
    }
}

void fillPixelGuarded(Surface& surface, i32 x, i32 y, Color color) {
    if (0 > x || x >= surface.width) return; // x ∈ [0, width)
    if (0 > y || y >= surface.height) return; // y ∈ [0, height)

    fillPixel(surface, x, y, color);
}

} // namespace

