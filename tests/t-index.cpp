#include "t-index.h"
#include "test_runner.h"

core::AllocatorId testAllocatorsStore[RegisteredAllocators::RA_SENTINEL] = {};
auto allTestAllocators = core::Memory<const core::AllocatorId>(testAllocatorsStore, CORE_C_ARRLEN(testAllocatorsStore));

static i32 g_skipMarkerRan = 0;
static i32 g_onlyBlockedRan = 0;

i32 test_basic_pass(const TestRunParams&) {
    CT_CHECK(true);
    return 0;
}

i32 test_skip_marker(const TestRunParams&) {
    g_skipMarkerRan++;
    return 0;
}

i32 test_skip_verifier(const TestRunParams&) {
    CT_CHECK(g_skipMarkerRan == 0, "skip marker test should not have run");
    return 0;
}

i32 test_only_runner(const TestRunParams& params) {
    CT_CHECK(params.actx != nullptr, "allocator context should be set");
    CT_CHECK(g_onlyBlockedRan == 0, "non-only test should not have run");
    return 0;
}

i32 test_only_blocked(const TestRunParams&) {
    g_onlyBlockedRan++;
    return 0;
}

i32 test_user_data(const TestRunParams& params) {
    CT_CHECK(params.userData != nullptr, "user data should be set");
    auto expected = reinterpret_cast<const char*>(params.userData);
    CT_CHECK(core::cstrLen(expected) == core::cstrLen("userdata"), "unexpected user data");
    return 0;
}

i32 runAllTests() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Set all allocators array
    for (u32 i = RegisteredAllocators::RA_DEFAULT; i < RegisteredAllocators::RA_SENTINEL; i++) {
        testAllocatorsStore[i] = i;
    }

    TestRunner<10> testRunner = {};
    testRunner.useAnsiColors = true;

    core::AllocatorId defaultAllocatorIds[] = { core::DEFAULT_ALLOCATOR_ID };
    auto defaultOnlyAllocators = core::Memory<const core::AllocatorId>(defaultAllocatorIds, CORE_C_ARRLEN(defaultAllocatorIds));

    testRunner.addTestGroup({
        .name = "Runner skip tests",
        .allocatorsToUse = defaultOnlyAllocators,
    })
        .addTest({
            .name = "skip marker (should not run)",
            .testFunction = test_skip_marker,
            .skip = true,
        })
        .addTest({
            .name = "skip verifier",
            .testFunction = test_skip_verifier,
        })
        .addTest({
            .name = "user data pass-through",
            .testFunction = test_user_data,
            .userData = reinterpret_cast<void*>(const_cast<char*>("userdata")),
        });

    testRunner.addTestGroup({
        .name = "Runner only tests",
        .allocatorsToUse = defaultOnlyAllocators,
    })
        .addTest({
            .name = "only runner",
            .testFunction = test_only_runner,
            .only = true,
        })
        .addTest({
            .name = "only blocked (should not run)",
            .testFunction = test_only_blocked,
        });

    testRunner.addTestGroup({
        .name = "Example group 1",
        .allocatorsToUse = allTestAllocators,
    })
        .addTest({
            .name = "test 1",
            .testFunction = test_basic_pass,
            .detectLeaks = true,
            .expectZeroAllocationsInGlobalAllocator = true,
        });

    testRunner.runAllTestGroups();

    return 0;
}
