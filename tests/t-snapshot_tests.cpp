#include "t-index.h"
#include "test_runner.h"
#include "test_utils.h"
#include "test_types.h"

#include "wavefront_files.h"
#include "surface.h"
#include "color.h"
#include "surface_renderer.h"
#include "model.h"
#include "face.h"
#include "tga_files.h"

// FIXME: Move something like this in core if notthing there is useful.
constexpr inline core::vec3i orthogonalProjection(core::vec3f normVec, i32 width, i32 height) {
    i32 x = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
    i32 y = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
    i32 z = i32((normVec.z() + 1.0f) * (255.f/2.0f));
    auto ret = core::v(x, y, z);
    return ret;
}

Model3D parseWavefrontFileToModel(const char* path, core::AllocatorContext& actx) {
    auto result = Wavefront::loadFile(path, Wavefront::WavefrontVersion::VERSION_3_0, actx);
    Assert(!result.hasErr(), "Failed to load Wavefront file");
    Wavefront::WavefrontObj obj = result.value();
    defer { obj.free(); };
    Model3D ret = createModelFromWavefrontObj(obj, actx);
    return ret;
}

Surface createTestSurface(
    i32 width,
    i32 height,
    PixelFormat pixelFormat,
    Origin origin,
    core::AllocatorContext& actx
) {
    i32 bpp = pixelFormatBytesPerPixel(pixelFormat);

    u8* buf = reinterpret_cast<u8*>(actx.alloc(addr_size(width*height*bpp), 1));
    Surface s = {
        .actx = &actx,
        .origin = origin,
        .pixelFormat = pixelFormat,
        .width = width,
        .height = height,
        .pitch = width * bpp,
        .data = buf,
    };

    return s;
}

void createTrueImageFile(const Surface& surface, const char* path) {
    TGA::CreateFileFromSurfaceParams tgaCreateParams = {
        .surface = surface,
        .path = path,
        .imageType = 2,
        .fileType = TGA::FileType::New,
    };
    core::Expect(TGA::createFileFromSurface(tgaCreateParams));
}

template <i32 N>
void genSnapshotFileName(const Surface& s, core::StaticPathBuilder<N>& pb) {
    pb.resetExtPart();

    char modifiedPathBuff[2048] = {};

    auto ret = core::format(
        modifiedPathBuff,
        CORE_C_ARRLEN(modifiedPathBuff),
        "{}-(w {} h {} format {}, origin {}).tga",
        pb.filePart(),
        s.width,
        s.height,
        pixelFormatToCstr(s.pixelFormat),
        originToCstr(s.origin)
    );

    AssertFmt(
        !ret.hasErr(),
        "Failed to create path; reason: format error '{}'",
        core::formatErrorToCStr(ret.err())
    );

    pb.setFilePart(core::sv(modifiedPathBuff, addr_size(ret.value())));
}

void updateSnapshot(const Surface& s, const TestSnapshotInfo* sinfo, const char* testName) {
    core::StaticPathBuilder<512> snapshotFilePb = {};
    snapshotFilePb.setFilePart(core::sv(sinfo->wavefrontInputFileFullPath));
    snapshotFilePb.setDirPart(core::sv(sinfo->snapshotDirectory));
    genSnapshotFileName(s, snapshotFilePb);

    if (sinfo->updateSnapshots) {
        createTrueImageFile(s, snapshotFilePb.fullPath());
    }
    else {
        core::StaticPathBuilder<512> outputFilePb = {};
        outputFilePb.setFilePart(core::sv(sinfo->wavefrontInputFileFullPath));
        outputFilePb.setDirPart(core::sv(sinfo->outputDirectory));
        outputFilePb.appendToDirPath(core::sv(testName));
        genSnapshotFileName(s, outputFilePb);

        createTrueImageFile(s, outputFilePb.fullPath());

        // Compare the output file with the snapshot:
        compareFilesBytewise(outputFilePb.fullPath(), snapshotFilePb.fullPath());
    }
}

i32 runDirectRasterizationSnapshotTest(const TestRunParams& params) {
    [[maybe_unused]] auto sinfo = reinterpret_cast<const TestSnapshotInfo*>(params.userData);

    Model3D model = parseWavefrontFileToModel(sinfo->wavefrontInputFileFullPath, *params.actx);
    defer { model.free(); };

    Surface s = createTestSurface(sinfo->width, sinfo->height, sinfo->pixelFormat, sinfo->origin, *params.actx);
    defer { s.free(); };

    fillRect(s, 0, 0, BLACK, s.width, s.height);

    for (addr_size i = 0; i < model.faces.len(); i++) {
        auto& f = model.faces[i];

        auto& v1 = model.vertices[f[0]];
        auto& v2 = model.vertices[f[1]];
        auto& v3 = model.vertices[f[2]];

        core::vec3i a = orthogonalProjection(v1.xyz(), s.width, s.height);
        core::vec3i b = orthogonalProjection(v2.xyz(), s.width, s.height);
        core::vec3i c = orthogonalProjection(v3.xyz(), s.width, s.height);

        fillTriangle(s, a, b, c, RED, GREEN, BLUE);
        strokeTriangleFast(s, a.xy(), b.xy(), c.xy(), WHITE);
    }

    updateSnapshot(s, sinfo, params.name);

    return 0;
}
