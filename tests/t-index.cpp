#include "t-index.h"
#include "test_runner.h"

i32 runAllTests() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Set all allocators array
    core::AllocatorId testAllocatorsStore[RegisteredAllocators::RA_SENTINEL] = {};
    auto allTestAllocators = core::Memory<const core::AllocatorId>(testAllocatorsStore, CORE_C_ARRLEN(testAllocatorsStore));
    for (u32 i = RegisteredAllocators::RA_DEFAULT; i < RegisteredAllocators::RA_SENTINEL; i++) {
        testAllocatorsStore[i] = i;
    }

    TestRunner<10> testRunner = {};
    testRunner.useAnsiColors = true;

    constexpr TestCreateInfo wavefrontTests[] = {
        { .name = FN_NAME_TO_CPTR(runWavefrontTestsSuite), .testFunction = runWavefrontTestsSuite },
    };
    constexpr TestCreateInfo tgaTests[] = {
        { .name = FN_NAME_TO_CPTR(runTgaTestsSuite), .testFunction = runTgaTestsSuite },
    };

    struct TestGroupTableEntry {
        TestGroupCreateInfo group;
        core::Memory<const TestCreateInfo> tests;
    };

    TestGroupTableEntry testGroups[] = {
        {
            .group = { .name = "Wavefront Test Suite", .allocatorsToUse = allTestAllocators },
            .tests = { wavefrontTests, CORE_C_ARRLEN(wavefrontTests) }
        },
        {
            .group = { .name = "TGA Test Suite", .allocatorsToUse = allTestAllocators },
            .tests = { tgaTests, CORE_C_ARRLEN(tgaTests) }
        },
    };

    for (auto& entry : testGroups) {
        auto& group = testRunner.addTestGroup(entry.group);
        for (addr_size i = 0; i < entry.tests.len(); i++) {
            group.addTest(entry.tests[i]);
        }
    }

    testRunner.runAllTestGroups();

    return 0;
}
