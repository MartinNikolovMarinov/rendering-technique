#include "t-index.h"
#include "test_runner.h"

constexpr const char* SNAPSHOT_DIRECTORY = TEST_ASSETS_DIRECTORY "/snapshots";
constexpr const char* TEST_OUTPUT_DIRECTORY = TEST_ASSETS_DIRECTORY "/test_output_directory";

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

    constexpr TestSnapshotInfo testSnapshotInfos[] = {
        {
            .wavefrontInputFile = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_triangle.obj",
            .snapshotDirectory = SNAPSHOT_DIRECTORY,
            .updateSnapshots = false
        }
    };

    TestCreateInfo wavefrontTests[] = {
        { .name = FN_NAME_TO_CPTR(runWavefrontTestsSuite), .testFunction = runWavefrontTestsSuite },
    };
    TestCreateInfo tgaTests[] = {
        { .name = FN_NAME_TO_CPTR(runTgaTestsSuite), .testFunction = runTgaTestsSuite },
        { .name = FN_NAME_TO_CPTR(runTgaTestsSuite), .testFunction = runTgaTestsSuite },
    };
    TestCreateInfo snapshotTests[] = {
        {
            .name = FN_NAME_TO_CPTR(runRenderSingleCenteredTriangle),
            .testFunction = runRenderSingleCenteredTriangle,
            .userData = &testSnapshotInfos[0]
        },
    };

    struct TestGroupTableEntry {
        TestGroupCreateInfo group;
        core::Memory<const TestCreateInfo> tests;
    };

    TestGroupTableEntry testGroups[] = {
        {
            .group = { .name = "Wavefront Tests Suite", .allocatorsToUse = allTestAllocators },
            .tests = { wavefrontTests, CORE_C_ARRLEN(wavefrontTests) }
        },
        {
            .group = { .name = "TGA Tests Suite", .allocatorsToUse = allTestAllocators },
            .tests = { tgaTests, CORE_C_ARRLEN(tgaTests) }
        },
        {
            .group = {
                .name = "Snapshot Tests Suite",
                .allocatorsToUse = allTestAllocators,
                .beforeAll = [] (const TestGroupRunParams&) {
                    // Create the test output directory if it does not exist.
                    bool exists = core::Unpack(core::fileExists(TEST_OUTPUT_DIRECTORY));
                    if (!exists) {
                        core::Expect(core::dirCreate(TEST_OUTPUT_DIRECTORY));
                    }

                    // Verify snapshot direcory exists:
                    exists = core::Unpack(core::fileExists(SNAPSHOT_DIRECTORY));
                    AssertFmt(
                        exists,
                        "Before allfailed because '{}' does not exist; provided path = '{}'",
                        FN_NAME_TO_CPTR(SNAPSHOT_DIRECTORY), SNAPSHOT_DIRECTORY
                    );
                },
                .afterAll = [](const TestGroupRunParams&) {
                    bool exists = core::Unpack(core::fileExists(TEST_OUTPUT_DIRECTORY));
                    AssertFmt(
                        exists,
                        "{} should exist on after all function call; provided path = '{}'",
                        FN_NAME_TO_CPTR(TEST_OUTPUT_DIRECTORY), TEST_OUTPUT_DIRECTORY
                    );

                    // Delete output directroy
                    // FIXME: Uncomment this later
                    // core::Expect(
                    //     core::dirDeleteRec<core::DEFAULT_ALLOCATOR_ID>(TEST_OUTPUT_DIRECTORY),
                    //     "AfterAll failed to delete {}",
                    //     FN_NAME_TO_CPTR(TEST_OUTPUT_DIRECTORY)
                    // );
                },
                .beforeEach = [] (const TestRunParams& params) {
                    auto sinfo = reinterpret_cast<const TestSnapshotInfo*>(params.userData);
                    const char* wavefrontInputFile = sinfo->wavefrontInputFile;
                    const char* testName = params.name;

                    bool exists = core::Unpack(core::fileExists(wavefrontInputFile));
                    AssertFmt(
                        exists,
                        "BeforeEach failed for test '{}'; reason: wavefront file '{}' does not exist!",
                        testName,
                        wavefrontInputFile
                    );

                    core::StaticPathBuilder<512> pathBuilder;
                    pathBuilder.setDirPath(TEST_OUTPUT_DIRECTORY);
                    pathBuilder.setFilePart(testName);
                    core::Expect(
                        core::dirCreate(pathBuilder.fullPath()),
                        "BeforeEach for test '{}' failed; reason: failed to create file '{}'",
                        testName,
                        pathBuilder.fullPath()
                    );
                    pathBuilder.reset();
                },
                // .afterEach = [] (const TestRunParams& params) {
                //     std::cout << "\t\t Clean Directory for '" << params.name << "'" << std::endl;
                // },
            },
            .tests = { snapshotTests, CORE_C_ARRLEN(snapshotTests) }
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
