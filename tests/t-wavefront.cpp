#include "t-index.h"
#include "testing/testing_framework.h"
#include "wavefront_files.h"

namespace {

struct VertexTestCase {
    addr_size index;
    core::vec4f expected;
    bool checkW;
};

i32 simpleVerticesTest(const core::testing::TestSuiteInfo& suiteInfo) {
    constexpr const char* vertices1_valid_path = TEST_ASSETS_DIRECTORY "/obj/vertices1_valid.obj";

    auto obj = core::Unpack(
        Wavefront::loadFile(vertices1_valid_path, Wavefront::WavefrontVersion::VERSION_3_0, *suiteInfo.actx),
        "Failed to load file: \"{}\"", vertices1_valid_path
    );
    defer { obj.free(); };

    CT_CHECK(obj.verticesCount == 8);

    constexpr VertexTestCase cases[] = {
        { 0, core::v(-1.0f, -1.0f, -1.0f, 0.0f), false },
        { 1, core::v(1.0f, -1.0f, -1.0f, 0.0f), false },
        { 2, core::v(1.0f, -1.0f, 1.25f, 0.0f), false },
        { 3, core::v(-1.5f, -1.0f, 99.0001f, 0.0f), false },
        { 4, core::v(-1.0f, -1.0f, -1.0f, 1.0f), true },
        { 5, core::v(1.0f, -1.0f, -1.0f, 0.5f), true },
        { 6, core::v(1.0f, -1.0f, 1.25f, 2.345f), true },
        { 7, core::v(-1.5f, -1.0f, 99.0f, 0.0001f), true },
    };

    i32 ret = core::testing::executeTestTable("simpleVerticesTest failed at: ", cases, [&](const auto& tc, const char* cErr) {
        CT_CHECK(tc.index < addr_size(obj.verticesCount), cErr);

        const core::vec4f& v = obj.vertices[tc.index];
        CT_CHECK(v.x() == tc.expected.x(), cErr);
        CT_CHECK(v.y() == tc.expected.y(), cErr);
        CT_CHECK(v.z() == tc.expected.z(), cErr);
        if (tc.checkW) {
            CT_CHECK(v.w() == tc.expected.w(), cErr);
        }

        return 0;
    });
    CT_CHECK(ret == 0);

    return 0;
}

} // namespace

i32 runWavefrontTestsSuite(const core::testing::TestSuiteInfo& suiteInfo) {
    using namespace core::testing;

    TestInfo tInfo = createTestInfo(suiteInfo);

    tInfo.name = FN_NAME_TO_CPTR(simpleVerticesTest);
    if (runTest(tInfo, simpleVerticesTest, suiteInfo) != 0) { return -1; }

    return 0;
}
