#include "t-index.h"
#include "tga_files.h"
#include "surface.h"

namespace {

constexpr const char* TRUE_IMAGE_TYPE_VALID_DIRECTORY = TEST_ASSETS_DIRECTORY "/tga/true_image_type_valid";

i32 validTrueImageFilesCanBeReadTest(const core::testing::TestSuiteInfo& suiteInfo) {
    struct Clojure {
        core::StaticPathBuilder<255> path;
        const core::testing::TestSuiteInfo& suiteInfo;
    };

    core::StaticPathBuilder<255> path;
    path.setDirPath(TRUE_IMAGE_TYPE_VALID_DIRECTORY);

    core::DirWalkCallback listWalk = [](const core::DirEntry& de, addr_size, void* userData) -> bool {
        Clojure* clojure = reinterpret_cast<Clojure*>(userData);
        auto& pbuilder = clojure->path;
        auto& sInfo = clojure->suiteInfo;

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

    Clojure clojure = { .path = path, .suiteInfo = suiteInfo };
    core::dirWalk(TRUE_IMAGE_TYPE_VALID_DIRECTORY, listWalk, &clojure);

    return 0;
}

} // namespace

i32 runTgaTestsSuite(const core::testing::TestSuiteInfo& suiteInfo) {
    using namespace core::testing;

    i32 ret = 0;
    TestInfo tInfo = createTestInfo(suiteInfo);

    tInfo.name = FN_NAME_TO_CPTR(trueImageTypeTest);
    if (runTest(tInfo, validTrueImageFilesCanBeReadTest, suiteInfo) != 0) { return -1; }

    return ret;
}
