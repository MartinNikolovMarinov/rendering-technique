#pragma once

#include "core_init.h"

#include <iostream>

// TODO: [TEST_RUNNER] Implement these as needed:
//  * Add a before and after functions for specific tests.

struct TestRunParams;
struct TestGroupRunParams;
struct TestCreateInfo;
struct TestGroup;
template<addr_size TTestGroupCount> struct TestRunner;

using TestFunction = i32 (*)(const TestRunParams& params);
using BeforeAllTestsFunction = void (*)(const TestGroupRunParams& params);
using AfterAllTestsFunction = void (*)(const TestGroupRunParams& params);
using BeforeEachTestFunction = void (*)(const TestRunParams& params);
using AfterEachTestFunction = void (*)(const TestRunParams& params);

namespace detail {

constexpr const char* passedOrFailedStr(bool passed, bool useAnsiColors) {
    if (useAnsiColors) {
        return passed ? ANSI_GREEN("PASSED") : ANSI_RED("FAILED");
    }
    return passed ? "PASSED" : "FAILED";
}

} // namespace detail

struct TestRunParams {
    const char* name = nullptr;
    core::AllocatorContext* actx = nullptr;
    const void* userData = nullptr;
};

struct TestGroupRunParams {
    const char* groupName = nullptr;
    core::Memory<const core::AllocatorId> allocatorsToUse = {};
    i32 testsCount = 0;
};

struct TestCreateInfo {
    const char* name = nullptr;
    TestFunction testFunction = nullptr;
    bool expectZeroAllocationsInGlobalAllocator = true;
    bool only = false;
    bool skip = false;
    const void* userData = nullptr;
};

struct TestGroupCreateInfo {
    const char* name = nullptr;
    core::Memory<const core::AllocatorId> allocatorsToUse = {};
    bool groupOnly = false;
    bool groupSkip = false;
    BeforeAllTestsFunction beforeAll = nullptr;
    AfterAllTestsFunction afterAll = nullptr;
    BeforeEachTestFunction beforeEach = nullptr;
    AfterEachTestFunction afterEach = nullptr;
};

struct TestGroup {

private:
    template<addr_size TTestGroupCount> friend struct TestRunner;

    struct Test {
        i32 testNumber;

        bool only;
        bool skip;
        bool trackMemory;
        bool detectLeaks;
        bool expectZeroAllocationsInGlobalAllocator;

        TestRunParams testRunParams;
        TestFunction testFunction;
    };

public:

    TestGroup& addTest(const TestCreateInfo& info);
    [[nodiscard]] i32 runTestGroup(i32& testCounter, bool useAnsiColors, u64 freq);

private:
    [[nodiscard]] u64 beginTest(Test& test);
    void endTest(
        Test& test,
        i32 returnCode,
        bool useAnsiColors,
        addr_size allocatedBefore,
        addr_size inUseBefore,
        addr_size globalAllocatedBefore,
        u64 startTsc,
        u64 freq
    );

    bool groupOnly = false;
    bool groupSkip = false;
    const char* name = nullptr;
    BeforeAllTestsFunction beforeAll = nullptr;
    AfterAllTestsFunction afterAll = nullptr;
    BeforeEachTestFunction beforeEach = nullptr;
    AfterEachTestFunction afterEach = nullptr;

    core::Memory<const core::AllocatorId> allocatorsToUse;
    core::ArrStatic<Test, 255> tests;
};

template<addr_size TTestGroupCount>
struct TestRunner {
    bool useAnsiColors = true;

    TestGroup& addTestGroup(const TestGroupCreateInfo& info) {
        TestGroup testGroup = {};
        testGroup.groupOnly = info.groupOnly;
        testGroup.groupSkip = info.groupSkip;
        testGroup.name = info.name;
        testGroup.beforeAll = info.beforeAll;
        testGroup.afterAll = info.afterAll;
        testGroup.beforeEach = info.beforeEach;
        testGroup.afterEach = info.afterEach;
        testGroup.allocatorsToUse = info.allocatorsToUse;
        testGroup.tests = {};
        testGroups.push(std::move(testGroup));
        return testGroups.last();
    }

    void runAllTestGroups() {
        u64 freq = core::getCPUFrequencyHz();
        Panic(freq != 0, "[BUG] CPU frequency is 0");

        bool hasOnly = core::forAny(testGroups, [](const TestGroup& t, addr_size) {
            return t.groupOnly == true && t.groupSkip == false;
        });

        i32 testCounter = 1;
        for (addr_size i = 0; i < testGroups.len(); i++) {
            TestGroup& testGroup = testGroups[i];

            // At least one test has an only flag set, therfore ignore tests that have 'only=false'.
            if (hasOnly && !testGroup.groupOnly) {
                skippedTestGroup(testGroup.name);
                continue;
            }
            if (testGroup.groupSkip) {
                skippedTestGroup(testGroup.name);
                continue;
            }

            u64 groupStartTsc = beginTestGroup(testGroup.name);
            i32 testGroupResult = testGroup.runTestGroup(testCounter, useAnsiColors, freq);
            endTestGroup(testGroup.name, testGroupResult, groupStartTsc, freq);

            AssertFmt(testGroupResult == 0, "Test {} failed", testGroup.name);
        }
    }

private:
    void skippedTestGroup(const char* suiteName) {
        std::cout
            << (useAnsiColors ? ANSI_YELLOW("[SUITE SKIPPED] ") : "[SUITE SKIPPED] ")
            << suiteName
            << std::endl;
    }

    u64 beginTestGroup(const char* suiteName) {
        std::cout << "[SUITE RUNNING] " << suiteName << std::endl;
        return core::getPerfCounter();
    }

    void endTestGroup(const char* suiteName, i32 returnCode, u64 startTsc, u64 freq) {
        std::cout << "[SUITE " << detail::passedOrFailedStr(returnCode == 0, useAnsiColors) << "] " << suiteName;

        auto endTsc = core::getPerfCounter();
        auto deltaTimeNs = u64(core::CORE_SECOND * (f64(endTsc - startTsc) / f64(freq)));

        char elapsedTimeBuffer[256];
        std::cout << " [ ";
        std::cout << "time: " << core::testing::elapsedTimeToStr(elapsedTimeBuffer, deltaTimeNs);
        std::cout << " ]";

        std::cout << std::endl;
    }

    core::ArrStatic<TestGroup, TTestGroupCount> testGroups;
};
