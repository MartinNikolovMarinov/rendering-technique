#include "t-index.h"
#include "test_runner.h"
#include "tga_files.h"
#include "surface.h"

namespace {

constexpr const char* TRUE_COLOR_TYPE_VALID_DIRECTORY = TEST_ASSETS_DIRECTORY "/tga/true_color_type_valid_image_type_2";

i32 validTrueImageFilesCanBeReadTest(const TestRunParams& params) {
    struct Clojure {
        core::StaticPathBuilder<255> path;
        const TestRunParams& params;
    };

    core::StaticPathBuilder<255> path;
    path.setDirPath(TRUE_COLOR_TYPE_VALID_DIRECTORY);

    core::DirWalkCallback listWalk = [](const core::DirEntry& de, addr_size, void* userData) -> bool {
        Clojure* clojure = reinterpret_cast<Clojure*>(userData);
        auto& pbuilder = clojure->path;
        auto& sInfo = clojure->params;

        pbuilder.resetFilePart();

        if (de.type == core::FileType::Regular) {
            pbuilder.setFilePart(de.name);

            TGA::TGAImage tgaImage;
            {
                auto res = TGA::loadFile(pbuilder.fullPath(), *sInfo.actx);
                AssertFmt(!res.hasErr(), "Failed to load file: \"{}\"", pbuilder.fullPath());
                tgaImage = std::move(res.value());
            }
            defer { tgaImage.free(); };

            const TGA::Header* header = nullptr;
            {
                auto res = tgaImage.header(header);
                AssertFmt(!res.hasErr(), "Failed to get image header; file path: \"{}\"", pbuilder.fullPath());
            }

            Surface surface;
            {
                auto res = TGA::createSurfaceFromTgaImage(tgaImage, *sInfo.actx);
                AssertFmt(!res.hasErr(), "Failed to create a surface from image; file path: \"{}\"", pbuilder.fullPath());
                surface = std::move(res.value());
            }
            defer { surface.free(); };
        }

        return true;
    };

    Clojure clojure = { .path = path, .params = params };
    core::dirWalk(TRUE_COLOR_TYPE_VALID_DIRECTORY, listWalk, &clojure);

    return 0;
}

} // namespace

i32 runTgaTestsSuite(const TestRunParams& params) {
    if (validTrueImageFilesCanBeReadTest(params) != 0) { return -1; }
    return 0;
}

i32 runRenderSingleCenteredTriangle(const TestRunParams& params) {
    // TODO: Write this test..
    // 1. I should write a before each in the group for all these tests to clean the temporary directory for snapshots.
    // 2. Read the input wavefront file and parse it.
    // 3. Render it to the temporary output directory as tga.
    // 4. Store the snapshots if this is a request for update.
    // 5. Or verify that they match bit-by-bit. (I can't think of a less painful way to do this sadly ..)

    [[maybe_unused]] auto sinfo = reinterpret_cast<const TestSnapshotInfo*>(params.userData);

    return 0;
}
