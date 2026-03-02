#include "core_init.h"
#include "tga_files.h"
#include "surface.h"
#include "log_utils.h"
#include "debug_rendering.h"
#include "surface_renderer.h"
#include "color.h"
#include "wavefront_files.h"

core::vec2f apply2DTransformation(core::vec2f v) {
    // Composition/order notes (direct vector ops):
    // operations execute top-to-bottom exactly as written in this function.

    // Scale:
    {
        core::vec2f scaledXY = core::scale(v.xy(), core::v(2.f, 0.5f));
        v.x() = scaledXY.x();
        v.y() = scaledXY.y();
    }

    // Rotate:
    {
        core::vec2f rotatedXY = core::rotate(v.xy(), core::vec2f::uniform(0.f), core::degToRad(90));
        v.x() = rotatedXY.x();
        v.y() = rotatedXY.y();
    }

    // Translate:
    {
        core::vec2f translatedXY = core::translate(v.xy(), core::vec2f::uniform(-0.5f));
        v.x() = translatedXY.x();
        v.y() = translatedXY.y();
    }

    // Skew:
    {
        core::vec2f skewedXY = core::skewX(v.xy(), core::degToRad(20));
        v.x() = skewedXY.x();
        v.y() = skewedXY.y();
    }

    return v;
}

core::vec2f apply2DHomogeneousCoordinatesTransformation(core::vec2f v) {
    core::mat3f m = core::mat3f::identity();

    // Compose rotation into matrix.
    {
        core::radians a = core::degToRad(70);
        core::vec2f pivot = core::v(0.f, 0.f);
        m = core::rotate(m, pivot, a);
    }

    // Compose translation into matrix.
    {
        core::vec2f t = core::v(0.2f, 0.3f);
        m = core::translate(m, t);
    }

    // Compose scaling into matrix.
    {
        core::vec2f s = core::v(1.3f, 0.5f);
        m = core::scale(m, s);
    }

    // Compose X skewing into matrix.
    {
        core::radians a = core::degToRad(10);
        m = core::skewX(m, a);
    }

    // Compose Y skewing into matrix.
    {
        core::radians a = core::degToRad(5);
        m = core::skewY(m, a);
    }

    // Composition/order notes (column vectors, post-multiply):
    // m = I * R * T * S * Kx * Ky
    // ret_h = m * [x, y, 1]^T = R * (T * (S * (Kx * (Ky * [x, y, 1]^T))))
    // So execution order on the vector is right-to-left:
    // 1) Y-skew, 2) X-skew, 3) scale, 4) translation, 5) rotation.
    // Because multiple non-commuting transforms are composed, the expanded x'/y' formulas
    // are longer and are best read directly from matrix m.
    core::vec3f ret = m * core::v(v.x(), v.y(), 1.0f);
    return ret.xy();
}

void playaroundWith2DTransformations(const char* path) {
    //==================================================================================================================
    // Boring initialization code
    //==================================================================================================================

    constexpr PixelFormat f = PixelFormat::BGR888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(f);

    constexpr addr_size WIDTH = 1000;
    constexpr addr_size HEGHT = 1000;

    u8 buf[WIDTH*HEGHT*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = f;
    s.width = WIDTH;
    s.height = HEGHT;
    s.pitch = s.width * bpp;
    s.data = buf;

    defer { s.free(); };

    constexpr const char* rectWithArrowOjbPath = ASSETS_DIRECTORY "/test_assets/obj/simple/rectangle_with_arrow.obj";

    auto objFile = Unpack(Wavefront::loadFile(rectWithArrowOjbPath, Wavefront::WavefrontVersion::VERSION_3_0));
    defer { objFile.free(); };

    //==================================================================================================================
    // Apply transformations to vertices
    //==================================================================================================================

    for (addr_size i = 0; i < objFile.vertices.at; i++) {
        auto& v = objFile.vertices[i];

        // Initial scale down to have more room to work with
        {
            core::vec2f scaledXY = core::scale(v.xy(), core::vec2f::uniform(0.4f));
            v.x() = scaledXY.x();
            v.y() = scaledXY.y();
        }

        // auto res = apply2DTransformation(v.xy());
        // v.x() = res.x();
        // v.y() = res.y();

        auto res = apply2DHomogeneousCoordinatesTransformation(v.xy());
        v.x() = res.x();
        v.y() = res.y();
    }

    //==================================================================================================================
    // Render to Surface
    //==================================================================================================================

    fillRect(s, 0, 0, BLACK, s.width, s.height);

    auto orthogonalProjection = [](core::vec3f normVec, i32 width, i32 height) -> core::vec3i {
        i32 x = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
        i32 y = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
        i32 z = i32((normVec.z() + 1.0f) * (255.f/2.0f));
        auto ret = core::v(x, y, z);
        return ret;
    };

    for (addr_size i = 0; i < objFile.faces.at; i++) {
        auto& vf = objFile.faces[i].v();

        core::vec4f& v1 = objFile.vertices[vf[0] - 1];
        core::vec4f& v2 = objFile.vertices[vf[1] - 1];
        core::vec4f& v3 = objFile.vertices[vf[2] - 1];

        core::vec3i a = orthogonalProjection(v1.xyz(), s.width, s.height);
        core::vec3i b = orthogonalProjection(v2.xyz(), s.width, s.height);
        core::vec3i c = orthogonalProjection(v3.xyz(), s.width, s.height);

        fillTriangle(s, a.xy(), b.xy(), c.xy(), GRAY, GRAY, BLACK);
    }

    //==================================================================================================================
    // Save TGA File
    //==================================================================================================================

    TGA::CreateFileFromSurfaceParams params = {
        .surface = s,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    Expect(TGA::createFileFromSurface(params));
}

i32 main() {
    [[maybe_unused]] const char* output = OUT_DIRECTORY "/output.tga";
    [[maybe_unused]] const char* depthPathOutput =  OUT_DIRECTORY "/depth-output.tga";
    [[maybe_unused]] const char* inputFile = ASSETS_DIRECTORY "/test_assets/tga/fileformat/ubw8.tga";

    {
        coreInit(core::LogLevel::L_DEBUG);
        defer { coreShutdown(); };

        playaroundWith2DTransformations(output);

        logInfo("Done");
    }

    return 0;
}
