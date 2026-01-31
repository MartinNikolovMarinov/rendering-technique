#include "t-index.h"
#include "test_runner.h"

constexpr const char* SNAPSHOT_DIRECTORY = TEST_ASSETS_DIRECTORY "/snapshots";

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
            .wavefrontInputFile = TEST_ASSETS_DIRECTORY "/obj/01_triangle.obj",
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
                .beforeAll = [] (const TestGroupRunParams& params) {
                    std::cout << "\t\t Verify Directory Exists and Is Clean for '" << params.groupName << "'" << std::endl;
                },
                .beforeEach = [] (const TestRunParams& params) {
                    std::cout << "\t\t Verify Directory Clean for '" << params.name << "'" << std::endl;
                },
                .afterEach = [] (const TestRunParams& params) {
                    std::cout << "\t\t Clean Directory for '" << params.name << "'" << std::endl;
                },
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
