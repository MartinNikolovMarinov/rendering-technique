#include "t-index.h"

using namespace core::testing;

namespace {

core::AllocatorId allRegisteredAllocators[RegisteredAllocators::RA_SENTINEL - 1] = {};
auto allAllocators = core::Memory<const core::AllocatorId>(allRegisteredAllocators, CORE_C_ARRLEN(allRegisteredAllocators));

template<i32 N>
struct TestManager {
    using TestFn = i32 (*)(const core::testing::TestSuiteInfo&);

    // FIXME: Create an output directory. Make sure it's clean after each test run..

    struct TestSuite {
        TestSuiteInfo suiteInfo = TestSuiteInfo();
        bool only = false;
        bool skip = false;
        TestFn testFn = nullptr;
        core::Memory<const core::AllocatorId> allocators = {};
    };

    core::ArrStatic<TestSuite, N> testSuites;
    bool hasOnly = false;

    void addTest(TestSuite&& t) {
        Assert(testSuites.cap() > testSuites.len(), "overflow");
        if (t.only) hasOnly = true;
        testSuites.push(t);
    }

    i32 runTests() {
        for (addr_size i = 0; i < testSuites.len(); i++) {
            auto& ts = testSuites[i];

            if (hasOnly && !ts.only) continue;
            if (ts.skip) continue;

            TestSuiteInfo suteInfo = ts.suiteInfo;

            if (ts.allocators.empty()) {
                CT_CHECK(runTestSuite(suteInfo, ts.testFn) == 0);
            }
            else {
                suteInfo.expectZeroAllocations = false;
                u64 start = beginTestSuiteGroup(suteInfo);
                for (addr_size j = 0; j < ts.allocators.len(); j++) {
                    suteInfo.actx = &core::getAllocator(ts.allocators[j]);
                    i32 returnCode = executeTestSuteInGroup(suteInfo, ts.testFn);
                    CT_CHECK(returnCode == 0);
                }
                endTestSuiteGroup(suteInfo, 0, start);
            }
        }

        return 0;
    }
};

} // namespace

constexpr bool useAnsiColors = true;

i32 runAllTests() {
    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Set all allocators array
    for (auto i = RegisteredAllocators::RA_DEFAULT + 1; i < RegisteredAllocators::RA_SENTINEL; i++) {
        allRegisteredAllocators[i-1] = i;
    }

    TestManager<255> testManager;

    testManager.addTest({
        .suiteInfo = TestSuiteInfo("Wavefront Tests Suite", useAnsiColors),
        .only = false,
        .skip = false,
        .testFn = runWavefrontTestsSuite,
        .allocators = allAllocators
    });

    testManager.addTest({
        .suiteInfo = TestSuiteInfo("TGA Tests Suite", useAnsiColors),
        .only = false,
        .skip = false,
        .testFn = runTgaTestsSuite,
        .allocators = allAllocators
    });

    i32 ret = testManager.runTests();
    return ret;
}
