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
void fillLineClipped(Surface& surface, const ViewPort& bounds, i32 ax, i32 ay, i32 bx, i32 by, Color color);

ViewPort calcClippedViewport(
    i32 surfaceWidth, i32 surfaceHeight,
    const ViewPort* viewport,
    core::vec2i& a, core::vec2i& b, core::vec2i& c
);

} // namespace

void fillPixel(Surface& surface, i32 x, i32 y, Color color) {
    if (!surface.viewport().isInside(x, y)) {
        return;
    }

    i32 idx = y * surface.pitch + x * surface.bpp();

    Assert(surface.data != nullptr, "surface data is null");
    Assert(idx + surface.bpp() <= surface.size(), "pixel write past end of surface");

    SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
    setPixelFn(surface.data, idx, color);
}

void fillPixelLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color) {
    i32 x = viewport.min.x() + relX;
    i32 y = viewport.min.y() + relY;
    if (viewport.isInside(x, y)) {
        fillPixel(surface, x, y, color);
    }
}

void fillLine(Surface& surface, i32 ax, i32 ay, i32 bx, i32 by, Color color) {
    fillLineClipped(surface, surface.viewport(), ax, ay, bx, by, color);
}

void fillLineLocal(Surface& surface, const ViewPort& viewport, i32 arelX, i32 arelY, i32 brelX, i32 brelY, Color color) {
    i32 ax = viewport.min.x() + arelX;
    i32 ay = viewport.min.y() + arelY;
    i32 bx = viewport.min.x() + brelX;
    i32 by = viewport.min.y() + brelY;
    fillLineClipped(surface, viewport, ax, ay, bx, by, color);
}

namespace {

void fillLineClipped(Surface& surface, const ViewPort& bounds, i32 ax, i32 ay, i32 bx, i32 by, Color color) {
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

    ViewPort bbox = bounds;
    bbox.clampTo(0, surface.width, 0, surface.height);
    bx = core::core_min(bx, (transpose ? bbox.max.y() : bbox.max.x()) - 1);

    i32 y = ay;
    i32 ierror = 0;
    const i32 dx = bx - ax;
    const i32 dty2 = core::absGeneric(by - ay) * 2;
    const i32 dtx2 = dx * 2;
    const i32 ydir = by > ay ? 1 : -1;

    for (i32 x = ax; x <= bx; x++) {
        i32 writeX = transpose ? y : x;
        i32 writeY = transpose ? x : y;

        if (writeX >= bbox.min.x() && writeX < bbox.max.x() &&
            writeY >= bbox.min.y() && writeY < bbox.max.y()
        ) {
            i32 idx = writeY * surface.pitch + writeX * surface.bpp();
            Assert(idx + surface.bpp() <= surface.size(), "pixel write past end of surface");
            setPixelFn(surface.data, idx, color);
        }

        ierror += dty2;
        if (ierror > dx) {
            y += ydir;
            ierror -= dtx2;
        }
    }
}

} // namespace

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

void fillRectLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color, i32 width, i32 height) {
    i32 x = viewport.min.x() + relX;
    i32 y = viewport.min.y() + relY;
    i32 endX = x + width;
    i32 endY = y + height;
    Assert(width > 0 && height > 0, "rect has non-positive size");
    Assert(x >= viewport.min.x() && x < viewport.max.x(), "rect origin x out of viewport bounds");
    Assert(y >= viewport.min.y() && y < viewport.max.y(), "rect origin y out of viewport bounds");
    Assert(endX <= viewport.max.x(), "rect extends past viewport width");
    Assert(endY <= viewport.max.y(), "rect extends past viewport height");
    fillRect(surface, x, y, color, width, height);
}

void strokeRectLocal(Surface& surface, const ViewPort& viewport, i32 relX, i32 relY, Color color, i32 width, i32 height) {
    i32 x = viewport.min.x() + relX;
    i32 y = viewport.min.y() + relY;
    i32 endX = x + width;
    i32 endY = y + height;
    Assert(width > 0 && height > 0, "rect has non-positive size");
    Assert(x >= viewport.min.x() && x < viewport.max.x(), "rect origin x out of viewport bounds");
    Assert(y >= viewport.min.y() && y < viewport.max.y(), "rect origin y out of viewport bounds");
    Assert(endX <= viewport.max.x(), "rect extends past viewport width");
    Assert(endY <= viewport.max.y(), "rect extends past viewport height");
    strokeRect(surface, x, y, color, width, height);
}

void strokeViewport(Surface& surface, const ViewPort& viewport, Color color) {
    // viewport interval is [min,max)
    i32 x = viewport.min.x();
    i32 y = viewport.min.y();
    i32 endX = viewport.max.x() - 1;
    i32 endY = viewport.max.y() - 1;
    fillLine(surface, x, y, endX, y, color);
    fillLine(surface, endX, y, endX, endY, color);
    fillLine(surface, endX, endY, x, endY, color);
    fillLine(surface, x, endY, x, y, color);
}

//======================================================================================================================
// Triangle Rendering
//======================================================================================================================

namespace {

void fillTriangleBarycentric(
    Surface& surface,
    const DepthBuffer* depthBuffer,
    const ViewPort* viewport,
    core::vec2i a, core::vec2i b, core::vec2i c,
    f32 az, f32 bz, f32 cz,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 holeInsetRatio
) {
    if (depthBuffer) {
        Assert(surface.width == depthBuffer->width);
        Assert(surface.height == depthBuffer->height);
    }

    ViewPort bbox = calcClippedViewport(surface.width, surface.height, viewport, a, b, c);

    f32 totalArea = core::calcTriangleAreaF32(a, b, c);
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x < bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y < bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.x(), b.y(), c.x(), c.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.x(), c.y(), a.x(), a.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.x(), a.y(), b.x(), b.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            // Check point against depth buffer:
            if (depthBuffer) {
                f32 z = alpha * f32(az) + beta * f32(bz) + gamma * f32(cz);
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
    const ViewPort* viewport,
    core::vec2i a, core::vec2i b, core::vec2i c,
    f32 az, f32 bz, f32 cz
) {
    // Calculate triangle bbox and clap to either the surface or the viewport:
    ViewPort bbox = calcClippedViewport(depthBuffer.width, depthBuffer.height, viewport, a, b, c);

    f32 totalArea = core::calcTriangleAreaF32(a, b, c);
    if (core::absGeneric(totalArea) < 1) {
        // Trying to draw triangle with area less than a pixel
        return;
    }

    // TODO: [PERFORMANCE] Parallelize this loop:
    for (i32 x = bbox.min.x(); x < bbox.max.x(); x++) {
        for (i32 y = bbox.min.y(); y < bbox.max.y(); y++) {
            f32 alpha = core::calcTriangleAreaF32(x, y, b.x(), b.y(), c.x(), c.y()) / totalArea;
            f32 beta  = core::calcTriangleAreaF32(x, y, c.x(), c.y(), a.x(), a.y()) / totalArea;
            f32 gamma = core::calcTriangleAreaF32(x, y, a.x(), a.y(), b.x(), b.y()) / totalArea;

            if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f) {
                // negative barycentric coordinate => the pixel is outside the triangle
                continue;
            }

            f32 z = alpha * az + beta * bz + gamma * cz;
            f32 depth = depthBuffer.at(x, y);
            if (z <= depth) {
                continue;
            }
            depthBuffer.at(x, y) = z;
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

void strokeTriangleFastLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& color
) {
    fillLineLocal(surface, viewport, relA.x(), relA.y(), relB.x(), relB.y(), color);
    fillLineLocal(surface, viewport, relB.x(), relB.y(), relC.x(), relC.y(), color);
    fillLineLocal(surface, viewport, relC.x(), relC.y(), relA.x(), relA.y(), color);
}

void strokeTriangleInset(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
) {
    f32 clampedRatio = core::core_max(0.0f, core::core_min(boarderRatio, 1.0f));
    fillTriangleBarycentric(
        surface, nullptr, nullptr,
        a, b, c,
        0, 0, 0,
        colorA, colorB, colorC,
        clampedRatio);
}

void strokeTriangleInsetLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& colorA, const Color& colorB, const Color& colorC,
    f32 boarderRatio
) {
    f32 clampedRatio = core::core_max(0.0f, core::core_min(boarderRatio, 1.0f));
    fillTriangleBarycentric(
        surface, nullptr, &viewport,
        relA, relB, relC,
        0, 0, 0,
        colorA, colorB, colorC,
        clampedRatio);
}

void fillTriangle(
    Surface& surface,
    const core::vec2i& a, const core::vec2i& b, const core::vec2i& c,
    const Color& colorA, const Color& colorB, const Color& colorC
) {
    fillTriangleBarycentric(
        surface, nullptr, nullptr,
        a, b, c,
        0, 0, 0,
        colorA, colorB, colorC,
        0.0f);
}

void fillTriangleLocal(
    Surface& surface,
    const ViewPort& viewport,
    const core::vec2i& relA, const core::vec2i& relB, const core::vec2i& relC,
    const Color& colorA, const Color& colorB, const Color& colorC
) {
    fillTriangleBarycentric(
        surface, nullptr, &viewport,
        relA, relB, relC,
        0, 0, 0,
        colorA, colorB, colorC,
        0.0f);
}

//======================================================================================================================
// Statefull Rendering Section
//======================================================================================================================

namespace {

struct ProjectedVertex {
    core::vec2i p;
    f32 z;
};

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

constexpr inline void assertViewportIsWellFormed(const ViewPort& viewport) {
    Assert(viewport.min.x() >= 0 && viewport.min.y() >= 0, "viewport min must be non-negative");
    Assert(viewport.max.x() > viewport.min.x(), "viewport width must be positive");
    Assert(viewport.max.y() > viewport.min.y(), "viewport height must be positive");
}

constexpr inline void assertViewportFitsSurface(const ViewPort& viewport, const Surface& surface) {
    assertViewportIsWellFormed(viewport);
    Assert(viewport.max.x() <= surface.width, "viewport extends past surface width");
    Assert(viewport.max.y() <= surface.height, "viewport extends past surface height");
}

constexpr inline void assertViewportFitsDepthBuffer(const ViewPort& viewport, const DepthBuffer& depthBuffer) {
    assertViewportIsWellFormed(viewport);
    Assert(viewport.max.x() <= depthBuffer.width, "viewport extends past depth buffer width");
    Assert(viewport.max.y() <= depthBuffer.height, "viewport extends past depth buffer height");
}

}

struct FrameState {
    DepthBuffer* depthBuffer;
    core::Memory<Vertex4f> vertices;
    core::Memory<Face3i> faces;
};

struct Renderer {
    core::AllocatorContext* actx;
    ViewPort viewport;
    bool wireframe;
    Surface* output;

    FrameState frameState;
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

void rendererSetViewport(RendererHandle r, ViewPort viewport) {
    Assert(r != nullptr, "renderer is null");
    assertViewportIsWellFormed(viewport);
    r->viewport = viewport;
}

void rendererSetWireframe(RendererHandle r, bool wireframe) {
    r->wireframe = wireframe;
}

void rendererSetOutput(RendererHandle r, Surface& output) {
    Assert(r != nullptr, "renderer is null");
    Assert(output.data != nullptr, "output surface data is null");
    r->output = &output;
}

void rendererBeginFrame(RendererHandle) {
}

void rendererClear(RendererHandle r, const Color& c) {
    Assert(r != nullptr, "renderer is null");
    Assert(r->output != nullptr, "renderer output is not set");
    assertViewportFitsSurface(r->viewport, *r->output);
    fillRectLocal(*r->output, r->viewport, 0, 0, c, r->viewport.width(), r->viewport.height());
}

void rendererSetVertexBuffer(RendererHandle r, core::Memory<Vertex4f> vertices) {
    r->frameState.vertices = vertices;
}

void rendererSetIndexBuffer(RendererHandle r, core::Memory<Face3i> indices) {
    r->frameState.faces = indices;
}

void rendererCalculateDepthBuffer(RendererHandle r, DepthBuffer& depthBuffer) {
    Assert(r != nullptr, "renderer is null");
    assertViewportFitsDepthBuffer(r->viewport, depthBuffer);
    r->frameState.depthBuffer = &depthBuffer;

    bool wireframeMode = r->wireframe;

    // Calculate depth buffer:
    if (!wireframeMode) {
        auto& vertices = r->frameState.vertices;
        auto& faces = r->frameState.faces;

        i32 width = r->viewport.width();
        i32 height = r->viewport.height();

        for (addr_size i = 0; i < faces.len(); i++) {
            auto& f = faces[i];

            core::vec4f& v1 = vertices[f[0]];
            core::vec4f& v2 = vertices[f[1]];
            core::vec4f& v3 = vertices[f[2]];

            // TODO: This projections is hardcoded for testing:
            ProjectedVertex a = orthogonalProjection(persp(rot(v1.xyz(), core::degToRad(30))), width, height);
            ProjectedVertex b = orthogonalProjection(persp(rot(v2.xyz(), core::degToRad(30))), width, height);
            ProjectedVertex c = orthogonalProjection(persp(rot(v3.xyz(), core::degToRad(30))), width, height);
            // ProjectedVertex a = orthogonalProjection(v1.xyz(), width, height);
            // ProjectedVertex b = orthogonalProjection(v2.xyz(), width, height);
            // ProjectedVertex c = orthogonalProjection(v3.xyz(), width, height);


            fillDepthBuffer(depthBuffer, &r->viewport, a.p, b.p, c.p, a.z, b.z, c.z);
        }
    }
}

void rendererColorPass(RendererHandle r) {
    Assert(r != nullptr, "renderer is null");
    Assert(r->output != nullptr, "renderer output is not set");

    auto& surface = *r->output;
    bool wireframe = r->wireframe;
    auto& vertices = r->frameState.vertices;
    auto& faces = r->frameState.faces;

    assertViewportFitsSurface(r->viewport, surface);

    DepthBuffer* depthBuffer = nullptr;
    if (!wireframe) {
        Assert(r->frameState.depthBuffer != nullptr, "depth buffer is required for filled rendering");
        depthBuffer = r->frameState.depthBuffer;
        assertViewportFitsDepthBuffer(r->viewport, *depthBuffer);
    }

    i32 width = r->viewport.width();
    i32 height = r->viewport.height();

    for (addr_size i = 0; i < faces.len(); i++) {
        auto& f = faces[i];

        core::vec4f& v1 = vertices[f[0]];
        core::vec4f& v2 = vertices[f[1]];
        core::vec4f& v3 = vertices[f[2]];

        // TODO: This projections is hardcoded for testing:
        ProjectedVertex a = orthogonalProjection(persp(rot(v1.xyz(), core::degToRad(30))), width, height);
        ProjectedVertex b = orthogonalProjection(persp(rot(v2.xyz(), core::degToRad(30))), width, height);
        ProjectedVertex c = orthogonalProjection(persp(rot(v3.xyz(), core::degToRad(30))), width, height);
        // ProjectedVertex a = orthogonalProjection(v1.xyz(), width, height);
        // ProjectedVertex b = orthogonalProjection(v2.xyz(), width, height);
        // ProjectedVertex c = orthogonalProjection(v3.xyz(), width, height);

        if (wireframe) {
            strokeTriangleFastLocal(surface, r->viewport, a.p, b.p, c.p, RED);
            fillPixelLocal(surface, r->viewport, a.p.x(), a.p.y(), WHITE);
            fillPixelLocal(surface, r->viewport, b.p.x(), b.p.y(), WHITE);
            fillPixelLocal(surface, r->viewport, c.p.x(), c.p.y(), WHITE);
        }
        else {
            Color color1 = randomColor();
            Color color2 = randomColor();
            Color color3 = randomColor();
            fillTriangleBarycentric(
                surface, depthBuffer, &r->viewport,
                a.p, b.p, c.p,
                a.z, b.z, c.z,
                color1, color2, color3,
                0.0f);
        }
    }
}

void rendererDepthColorPass(RendererHandle r) {
    Assert(r != nullptr, "renderer is null");
    Assert(r->output != nullptr, "renderer output is not set");
    Assert(r->frameState.depthBuffer != nullptr, "depth buffer is required for depth color pass");

    DepthBuffer& depthBuffer = *r->frameState.depthBuffer;
    assertViewportFitsDepthBuffer(r->viewport, depthBuffer);

    auto& surface = *r->output;
    assertViewportFitsSurface(r->viewport, surface);

    i32 width = r->viewport.width();
    i32 height = r->viewport.height();

    for (i32 relY = 0; relY < height; relY++) {
        for (i32 relX = 0; relX < width; relX++) {
            // Offest triangle points with viewport:
            i32 x = r->viewport.min.x() + relX;
            i32 y = r->viewport.min.y() + relY;
            Assert(r->viewport.isInside(x, y));

            f32 d = depthBuffer.at(x, y);
            f32 t = core::clamp(d, 0.0f, 1.0f);
            u8 gray = u8(t * 255.0f);
            i32 idx = y * surface.pitch + x * surface.bpp();

            SetPixelFn setPixelFn = pickSetPixelFunction(surface.pixelFormat);
            setPixelFn(surface.data, idx, { .rgba = { gray, gray, gray, gray } });
        }
    }
}

void rendererEndFrame(RendererHandle r) {
    Assert(r != nullptr, "renderer is null");
    if (r->frameState.depthBuffer) {
        r->frameState.depthBuffer->clear(0);
    }
    r->frameState = {};
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

ViewPort calcClippedViewport(
    i32 surfaceWidth, i32 surfaceHeight,
    const ViewPort* viewport,
    core::vec2i& a, core::vec2i& b, core::vec2i& c
) {
    ViewPort bbox;
    if (viewport != nullptr) {
        // Offset triangle points with viewport:
        a = viewport->min + a;
        b = viewport->min + b;
        c = viewport->min + c;
        // Only after offset calculate the point's bbox:
        bbox = core::calcTriangleBBox(a, b, c);
        bbox.clampTo(viewport->min.x(), viewport->max.x(), viewport->min.y(), viewport->max.y());
    }
    else {
        bbox = core::calcTriangleBBox(a, b, c);
        bbox.clampTo(0, surfaceWidth, 0, surfaceHeight);
    }

    return bbox;
}

} // namespace
