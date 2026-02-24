#include "test_runner.h"

namespace {
void skippedTest(const char* testName, bool useAnsiColors);
} // namespace

TestGroup& TestGroup::addTest(const TestCreateInfo& info) {
    AssertFmt(info.testFunction, "Trying to create test '{}' without a test function!", info.name);
    Test test = {
        .testNumber = 0, // Will set this later!
        .only = info.only,
        .skip = info.skip,
        .trackMemory = false, // Will set this later!
        .detectLeaks = false, // Will set this later!
        .expectZeroAllocationsInGlobalAllocator = info.expectZeroAllocationsInGlobalAllocator,
        .testRunParams = {
            .name = info.name,
            .actx = nullptr, // Will set this later!
            .userData = info.userData,
        },
        .testFunction = info.testFunction
    };
    tests.push(std::move(test));
    return *this;
}

[[nodiscard]] i32 TestGroup::runTestGroup(i32& testCounter, i32& skippedTests, bool useAnsiColors, u64 freq) {
    bool hasOnly = core::forAny(tests, [](const Test& t, addr_size) {
        return t.only == true && t.skip == false;
    });

    auto groupHookRunParams = TestGroupRunParams {
        .groupName = name,
        .allocatorsToUse = allocatorsToUse,
        .testsCount = i32(tests.len()),
    };

    if (beforeAll) beforeAll(groupHookRunParams);
    defer { if (afterAll) afterAll(groupHookRunParams); };

    for (addr_size i = 0; i < tests.len(); i++) {
        Test& test = tests[i];

        AssertFmt(test.testFunction, "Test '{}' has no test function", test.testRunParams.name);

        // At least one test has an only flag set, therfore ignore tests that have 'only=false'.
        if (hasOnly && !test.only) {
            skippedTests++;
            skippedTest(test.testRunParams.name, useAnsiColors);
            continue;
        }
        if (test.skip) {
            skippedTests++;
            skippedTest(test.testRunParams.name, useAnsiColors);
            continue;
        }

        for (addr_size allocatorIdx = 0; allocatorIdx < allocatorsToUse.len(); allocatorIdx++) {
            auto& actx = core::getAllocator(allocatorsToUse[allocatorIdx]);
            test.testRunParams.actx = &actx;
            test.testNumber = testCounter++;
            test.trackMemory = actx.tracksMemory();
            test.detectLeaks = actx.canDetectLeaks();

            AssertFmt(test.testRunParams.actx, "Test '{}' has no allocator context", test.testRunParams.name);

            // IMPORTANT:
            // This is the exact correct spot for before each, because it might use the default allocator at some point,
            // which might cause some Zero Allocations expectations to fail. DO NOT MOVE!
            // Memory leaks in before each will also be detected; I consider this a feature not a bug.
            if (beforeEach) beforeEach(test.testRunParams);

            auto allocatedBefore = test.testRunParams.actx->totalMemoryAllocated();
            auto inUseBefore = test.testRunParams.actx->inUseMemory();
            auto& globalActx = core::getAllocator(core::DEFAULT_ALLOCATOR_ID);
            auto globalAllocatedBefore = globalActx.totalMemoryAllocated();

            u64 startTsc = beginTest(test);
            i32 testResult = test.testFunction(test.testRunParams);
            endTest(test, testResult, useAnsiColors, allocatedBefore, inUseBefore, globalAllocatedBefore, startTsc, freq);

            if (afterEach) afterEach(test.testRunParams);

            AssertFmt(testResult == 0, "Test {} failed", test.testRunParams.name);

            if (testResult != 0) {
                return testResult;
            }
        }
    }

    return 0;
}

[[nodiscard]] u64 TestGroup::beginTest(Test& test) {
    i32 testNumber = test.testNumber;
    const char* testName = test.testRunParams.name;
    const char* allocatorName = test.testRunParams.actx->name();
    std::cout << "\t[TEST " << "№ " << testNumber << " RUNNING] " << testName;
    if (allocatorName) {
        std::cout << " ( " << allocatorName << " )";
    }
    std::cout << '\n';
    return core::getPerfCounter();
}

void TestGroup::endTest(
    Test& test,
    i32 returnCode,
    bool useAnsiColors,
    addr_size allocatedBefore,
    addr_size inUseBefore,
    addr_size globalAllocatedBefore,
    u64 startTsc,
    u64 freq
) {
    char buff[256];
    const char* testName = test.testRunParams.name;
    const char* allocatorName = test.testRunParams.actx->name();
    i32 testNumber = test.testNumber;

    auto allocatedAfter = test.testRunParams.actx->totalMemoryAllocated();
    auto inUseAfter = test.testRunParams.actx->inUseMemory();
    auto& globalActx = core::getAllocator(core::DEFAULT_ALLOCATOR_ID);
    bool sameAsGlobalAllocator = test.testRunParams.actx == &globalActx;
    auto globalAllocatedAfter = globalActx.totalMemoryAllocated();
    auto endTsc = core::getPerfCounter();

    auto deltaAllocatedMemory = allocatedAfter - allocatedBefore;
    auto deltaInUseMemory = inUseAfter - inUseBefore;
    auto deltaGlobalAllocatedMemory = globalAllocatedAfter - globalAllocatedBefore;
    auto deltaTimeNs = u64(core::CORE_SECOND * (f64(endTsc - startTsc) / f64(freq)));

    std::cout << "\t[TEST " << "№ " << testNumber << " "
          << detail::passedOrFailedStr(returnCode == 0, useAnsiColors) << "] "
          << testName;
    if (allocatorName) {
        std::cout << " ( " << allocatorName << " )";
    }

    if (test.detectLeaks && deltaInUseMemory != 0) {
        std::cout << (useAnsiColors ? ANSI_RED(" !!LEAKED MEMORY!!") : " !!LEAKED MEMORY!!") << std::endl;
        AssertFmt(false, "Test {} failed; reason: MEMORY LEAK DETECTED", testName);
    }

    if (test.expectZeroAllocationsInGlobalAllocator &&
        !sameAsGlobalAllocator &&
        deltaGlobalAllocatedMemory > 0
    ) {
        std::cout << (useAnsiColors
            ? ANSI_RED(" !!UNEXPECTED DYNAMIC MEMORY USAGE!!")
            : " !!UNEXPECTED DYNAMIC MEMORY USAGE!!") << std::endl;

        AssertFmt(false,
            "Test {} failed; reason: Test core::expected Zero Allocations In Default Allocator But Allocated {}",
            testName,
            core::testing::memoryUsedToStr(buff, deltaGlobalAllocatedMemory)
        );
    }

    std::cout << " [ ";
    std::cout << "time: " << core::testing::elapsedTimeToStr(buff, deltaTimeNs);

    if (test.trackMemory) {
        std::cout << ", ";
        std::cout << "memory: {"
                << " allocated: " << core::testing::memoryUsedToStr(buff, deltaAllocatedMemory)
                << ", in_use: " << core::testing::memoryUsedToStr(buff, deltaInUseMemory)
                << " }";
    }

    std::cout << " ]" << std::endl;
}

namespace {

void skippedTest(const char* testName, bool useAnsiColors) {
    std::cout
        << "\t"
        << (useAnsiColors ? ANSI_YELLOW("[TEST SKIPPED] ") : "[TEST SKIPPED] ")
        << testName
        << std::endl;
}

} // namespace
