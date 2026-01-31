#include "t-index.h"
#include "test_runner.h"
#include "wavefront_files.h"
#include "surface.h"
#include "color.h"
#include "surface_renderer.h"
#include "model.h"
#include "face.h"
#include "tga_files.h"

void compareFilesBytewise(const char* fileA, const char* fileB);

// FIXME: Move something like this in core if notthing there is useful.
constexpr inline core::vec3i orthogonalProjection(core::vec3f normVec, i32 width, i32 height) {
    i32 x = i32((normVec.x() + 1.0f) * (f32(width - 1)/2.0f));
    i32 y = i32((normVec.y() + 1.0f) * (f32(height - 1)/2.0f));
    i32 z = i32((normVec.z() + 1.0f) * (255.f/2.0f));
    auto ret = core::v(x, y, z);
    return ret;
}

i32 runRenderSingleCenteredTriangleTest(const TestRunParams& params) {
    // TODO: Simplify this to make it reusable!

    [[maybe_unused]] auto sinfo = reinterpret_cast<const TestSnapshotInfo*>(params.userData);

    auto result = Wavefront::loadFile(sinfo->wavefrontInputFile, Wavefront::WavefrontVersion::VERSION_3_0, *params.actx);
    CT_CHECK(!result.hasErr(), "Failed to load Wavefront file");

    Wavefront::WavefrontObj obj = result.value();
    defer { obj.free(); };
    Model3D model = createModelFromWavefrontObj(obj, *params.actx);
    defer { model.free(); };

    constexpr PixelFormat pixelFormat = PixelFormat::BGRA8888;
    constexpr i32 bpp = pixelFormatBytesPerPixel(pixelFormat);
    constexpr addr_size WIDTH = 800;
    constexpr addr_size HEIGHT = 800;

    static u8 buf[WIDTH*HEIGHT*bpp] = {};
    Surface s = Surface();
    s.actx = nullptr;
    s.origin = Origin::BottomLeft;
    s.pixelFormat = pixelFormat;
    s.width = WIDTH;
    s.height = HEIGHT;
    s.pitch = s.width * bpp;
    s.data = buf;
    defer { s.free(); };

    fillRect(s, 0, 0, BLACK, s.width, s.height);

    for (addr_size i = 0; i < model.faces; i++) {
        auto& f = model.faces[i];

        auto& v1 = obj.vertices[f[0]];
        auto& v2 = obj.vertices[f[1]];
        auto& v3 = obj.vertices[f[2]];

        core::vec3i a = orthogonalProjection(v1.xyz(), s.width, s.height);
        core::vec3i b = orthogonalProjection(v2.xyz(), s.width, s.height);
        core::vec3i c = orthogonalProjection(v3.xyz(), s.width, s.height);

        strokeTriangleFast(s, a.xy(), b.xy(), c.xy(), GREEN);
    }


    if (sinfo->updateSnapshots) {
        core::StaticPathBuilder<512> pathBuilder;
        pathBuilder.setDirPath(sinfo->snapshotDirectory);
        pathBuilder.setFilePart("triangle_render.tga");

        // Create the ouput file:
        TGA::CreateFileFromSurfaceParams tgaCreateParams = {
            .surface = s,
            .path = pathBuilder.fullPath(),
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(tgaCreateParams));

        pathBuilder.reset();
    }
    else {
        core::StaticPathBuilder<512> resultPathBuilder;
        resultPathBuilder.setDirPath(sinfo->outputDirectory);
        resultPathBuilder.setFilePart("triangle_render.tga");

        // Crate the output file:
        TGA::CreateFileFromSurfaceParams tgaCreateParams = {
            .surface = s,
            .path = resultPathBuilder.fullPath(),
            .imageType = 2,
            .fileType = TGA::FileType::New,
        };
        core::Expect(TGA::createFileFromSurface(tgaCreateParams));

        // Compare the two files bytewise:
        core::StaticPathBuilder<512> expectedPathBuilder;
        expectedPathBuilder.setDirPath(sinfo->snapshotDirectory);
        expectedPathBuilder.setFilePart("triangle_render.tga");

        compareFilesBytewise(resultPathBuilder.fullPath(), expectedPathBuilder.fullPath());

        resultPathBuilder.reset();
        expectedPathBuilder.reset();
    }

    return 0;
}

void compareFilesBytewise(const char* fileA, const char* fileB) {
    auto fileADesc = core::Unpack(
        core::fileOpen(fileA, core::OpenMode::Read),
        "Failed to open file '{}'",
        fileA
    );
    defer { core::Expect(core::fileClose(fileADesc)); };

    auto fileBDesc = core::Unpack(
        core::fileOpen(fileB, core::OpenMode::Read),
        "Failed to open file '{}'",
        fileB
    );
    defer { core::Expect(core::fileClose(fileBDesc)); };

    addr_size sizeA = core::Unpack(
        core::fileSize(fileADesc),
        "Failed to get size for file '{}'",
        fileA
    );
    addr_size sizeB = core::Unpack(
        core::fileSize(fileBDesc),
        "Failed to get size for file '{}'",
        fileB
    );

    AssertFmt(
        sizeA == sizeB,
        "Snapshot size mismatch: '{}' has {}, '{}' has {}",
        fileA, sizeA, fileB, sizeB
    );

    constexpr addr_size kChunkSize = 4096;
    u8 bufferA[kChunkSize];
    u8 bufferB[kChunkSize];
    addr_size remaining = sizeA;
    addr_size offset = 0;

    while (remaining > 0) {
        addr_size chunk = remaining < kChunkSize ? remaining : kChunkSize;
        addr_size readA = core::Unpack(
            core::fileRead(fileADesc, bufferA, chunk),
            "Failed to read file '{}'",
            fileA
        );
        addr_size readB = core::Unpack(
            core::fileRead(fileBDesc, bufferB, chunk),
            "Failed to read file '{}'",
            fileB
        );

        AssertFmt(
            readA == chunk,
            "Short read for '{}': expected {}, got {}",
            fileA, chunk, readA
        );
        AssertFmt(
            readB == chunk,
            "Short read for '{}': expected {}, got {}",
            fileB, chunk, readB
        );

        if (core::memcmp(bufferA, bufferB, chunk) != 0) {
            addr_size diffIdx = 0;
            for (; diffIdx < chunk; diffIdx++) {
                if (bufferA[diffIdx] != bufferB[diffIdx]) {
                    break;
                }
            }

            AssertFmt(
                false,
                "Snapshot mismatch at byte {}: '{}' has {}, '{}' has {}",
                offset + diffIdx,
                fileA,
                static_cast<u32>(bufferA[diffIdx]),
                fileB,
                static_cast<u32>(bufferB[diffIdx])
            );
        }

        remaining -= chunk;
        offset += chunk;
    }
}
