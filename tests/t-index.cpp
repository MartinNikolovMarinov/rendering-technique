#include "t-index.h"

#include "test_runner.h"
#include "test_types.h"
#include "test_utils.h"

#include "snapshot_test_cases_table.h"

void beforeAllSnapshotTests(const TestGroupRunParams&) {
    bool exists;

    // Create the test output directory if it does not exist.
    exists = core::Unpack(core::fileExists(TEST_OUTPUT_DIRECTORY));
    if (!exists) {
        core::Expect(core::dirCreate(TEST_OUTPUT_DIRECTORY));
    }

    // Verify snapshot direcory exists:
    exists = core::Unpack(core::fileExists(SNAPSHOT_ROOT_DIRECTORY));
    AssertFmt(
        exists,
        "BeforeAll failed because '{}' does not exist; provided path = '{}'",
        FN_NAME_TO_CPTR(SNAPSHOT_ROOT_DIRECTORY), SNAPSHOT_ROOT_DIRECTORY
    );
}

void afterAllSnapshotTests(const TestGroupRunParams&) {
    bool exists = core::Unpack(core::fileExists(TEST_OUTPUT_DIRECTORY));
    AssertFmt(
        exists,
        "{} should exist on after all function call; provided path = '{}'",
        FN_NAME_TO_CPTR(TEST_OUTPUT_DIRECTORY), TEST_OUTPUT_DIRECTORY
    );

    // Delete output directroy
    auto deleteRes = core::dirDeleteRec<core::DEFAULT_ALLOCATOR_ID>(TEST_OUTPUT_DIRECTORY);
    AssertFmt(
        !deleteRes.hasErr(),
        "AfterAll failed to delete {}; error code = {}",
        FN_NAME_TO_CPTR(TEST_OUTPUT_DIRECTORY),
        deleteRes.err()
    );
}

void beforeEachSnapshotTest(const TestRunParams& params) {
    auto sinfo = reinterpret_cast<const TestSnapshotInfo*>(params.userData);
    const char* wavefrontInputFile = sinfo->wavefrontInputFileFullPath;
    const char* testName = params.name;

    // Wavefront input file must exist
    bool exists = core::Unpack(core::fileExists(wavefrontInputFile));
    AssertFmt(
        exists,
        "BeforeEach failed for test '{}'; reason: wavefront file '{}' does not exist!",
        testName,
        wavefrontInputFile
    );

    // Create the output directory
    core::StaticPathBuilder<512> pathBuilder;
    pathBuilder.setDirPart(core::sv(TEST_OUTPUT_DIRECTORY));
    pathBuilder.setFilePart(core::sv(testName));
    core::Expect(
        core::dirCreate(pathBuilder.fullPath()),
        "BeforeEach for test '{}' failed; reason: failed to create file '{}'",
        testName,
        pathBuilder.fullPath()
    );
    pathBuilder.reset();
}

i32 runAllTests() {
    //==================================================================================================================
    // Initial Setup Code
    //==================================================================================================================

    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Set all allocators array
    core::AllocatorId testAllocatorsStore[RegisteredAllocators::RA_SENTINEL] = {};
    auto allTestAllocators = core::Memory<const core::AllocatorId>(testAllocatorsStore, CORE_C_ARRLEN(testAllocatorsStore));
    for (u32 i = RegisteredAllocators::RA_DEFAULT; i < RegisteredAllocators::RA_SENTINEL; i++) {
        testAllocatorsStore[i] = i;
    }

    //==================================================================================================================
    // Table Definitions for all Tests
    //==================================================================================================================

    TestCreateInfo wavefrontTests[] = {
        {
            .name = "Test Vertex Parsing",
            .testFunction = runWavefrontVerticesTest,
            .userData = TEST_ASSETS_DIRECTORY "/obj/vertices1_valid.obj",
        },
        {
            .name = "Test Faces Parsing",
            .testFunction = runWavefrontFacesTest,
            .userData = TEST_ASSETS_DIRECTORY "/obj/faces1_valid.obj",
        },
    };

    TestCreateInfo tgaTests[] = {
        {
            .name = "Create Surface From True Image Files (Image Type 2) Test",
            .testFunction = runCreateSurfaceFromTgaFilesInDirectoryTest,
            .userData = TEST_ASSETS_DIRECTORY "/tga/true_color_type_valid_image_type_2",
        },
        {
            .name = "Create Surface From Grayscale Image Files (Image Type 3) Test",
            .testFunction = runCreateSurfaceFromTgaFilesInDirectoryTest,
            .userData = TEST_ASSETS_DIRECTORY "/tga/gray_scale_image_type_3",
        },
    };

    struct TestGroupTableEntry {
        TestGroupCreateInfo group;
        core::Memory<const TestCreateInfo> tests;
    };

    TestGroupTableEntry testGroups[] = {
        {
            .group = { .name = "Wavefront Tests Suite", .allocatorsToUse = allTestAllocators, },
            .tests = { wavefrontTests, CORE_C_ARRLEN(wavefrontTests) }
        },
        {
            .group = { .name = "TGA Tests Suite", .allocatorsToUse = allTestAllocators, },
            .tests = { tgaTests, CORE_C_ARRLEN(tgaTests) }
        },
        {
            .group = {
                .name = "Snapshot Tests Suite",
                .allocatorsToUse = allTestAllocators,
                .beforeAll = beforeAllSnapshotTests,
                .afterAll = afterAllSnapshotTests,
                .beforeEach = beforeEachSnapshotTest,
            },
            .tests = { snapshotTests, CORE_C_ARRLEN(snapshotTests) },
        },
    };

    //==================================================================================================================
    // Initialize and Start the Test Runner
    //==================================================================================================================

    TestRunner<10> testRunner = {};
    testRunner.useAnsiColors = true;

    for (auto& entry : testGroups) {
        auto& group = testRunner.addTestGroup(entry.group);
        for (addr_size i = 0; i < entry.tests.len(); i++) {
            group.addTest(entry.tests[i]);
        }
    }

    testRunner.runAllTestGroups();

    return 0;
}
