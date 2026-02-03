#include "t-index.h"
#include "test_runner.h"
#include "tga_files.h"
#include "surface.h"


i32 runCreateSurfaceFromTgaFilesInDirectoryTest(TestRunParams& params) {
    const char* directory = reinterpret_cast<const char*>(params.userData);

    struct Clojure {
        core::StaticPathBuilder<255> path;
        TestRunParams& params;
    };

    core::StaticPathBuilder<255> path;
    path.setDirPart(core::sv(directory));

    core::DirWalkCallback listWalk = [](const core::DirEntry& de, addr_size, void* userData) -> bool {
        Clojure* clojure = reinterpret_cast<Clojure*>(userData);
        auto& pbuilder = clojure->path;
        auto& sInfo = clojure->params;

        pbuilder.resetFilePart();

        if (de.type == core::FileType::Regular) {
            pbuilder.setFilePart(core::sv(de.name));

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
    core::dirWalk(directory, listWalk, &clojure);

    return 0;
}
