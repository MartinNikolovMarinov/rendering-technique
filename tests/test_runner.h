#pragma once

#include "core_init.h"

const char* passedOrFailedStr(bool passed, bool useAnsiColors);

struct TestRunParams;
using TestFunction = i32 (*)(const TestRunParams& input);

struct TestRunParams {
    const char* name = nullptr;
    core::AllocatorContext* actx = nullptr;
    void* userData = nullptr;
};

struct Test {
    i32 testNumber;

    bool only = false;
    bool skip = false;
    bool trackMemory = true;
    bool detectLeaks = true;
    bool expectZeroAllocationsInGlobalAllocator = true;

    TestRunParams testRunParams = {};
    TestFunction testFunction = nullptr;
};

struct TestCreateInfo {
    const char* name = nullptr;
    TestFunction testFunction = nullptr;
    bool detectLeaks = true;
    bool expectZeroAllocationsInGlobalAllocator = true;
    bool only = false;
    bool skip = false;
    void* userData = nullptr;
};

struct TestGroup {
    bool groupOnly = false;
    bool groupSkip = false;
    const char* name = nullptr;

    core::Memory<const core::AllocatorId> allocatorsToUse;
    core::ArrStatic<Test, 255> tests;

    TestGroup& addTest(const TestCreateInfo& info);
    [[nodiscard]] i32 runTestGroup(i32& testCounter, bool useAnsiColors);

private:
    [[nodiscard]] u64 beginTest(Test& test);
    void endTest(
        Test& test,
        i32 returnCode,
        bool useAnsiColors,
        addr_size allocatedBefore,
        addr_size inUseBefore,
        addr_size globalAllocatedBefore,
        u64 startTsc
    );
};

struct TestGroupCreateInfo {
    const char* name = nullptr;
    core::Memory<const core::AllocatorId> allocatorsToUse = {};
    bool groupOnly = false;
    bool groupSkip = false;
};

template<addr_size TTestGroupCount>
struct TestRunner {
    core::ArrStatic<TestGroup, TTestGroupCount> testGroups;
    bool useAnsiColors = true;

    [[nodiscard]] TestGroup& addTestGroup(const TestGroupCreateInfo& info) {
        TestGroup testGroup = {
            .groupOnly = info.groupOnly,
            .groupSkip = info.groupSkip,
            .name = info.name,
            .allocatorsToUse = info.allocatorsToUse,
            .tests = {}
        };
        testGroups.push(std::move(testGroup));
        return testGroups.last();
    }

    void runAllTestGroups() {
        bool hasOnly = core::forAny(testGroups, [](const TestGroup& t, addr_size) {
            return t.groupOnly == true && t.groupSkip == false;
        });

        i32 testCounter = 0;
        for (addr_size i = 0; i < testGroups.len(); i++) {
            TestGroup& testGroup = testGroups[i];

            // At least one test has an only flag set, therfore ignore tests that have 'only=false'.
            if (hasOnly && !testGroup.groupOnly) {
                continue;
            }
            if (testGroup.groupSkip) {
                continue;
            }

            u64 groupStartTsc = beginTestGroup(testGroup.name);

            i32 testGroupResult = testGroup.runTestGroup(testCounter, useAnsiColors);
            AssertFmt(testGroupResult == 0, "Test {} failed", testGroup.name);

            endTestGroup(testGroup.name, testGroupResult, groupStartTsc);
        }
    }

private:
    u64 beginTestGroup(const char* suiteName) {
        std::cout << "[SUITE RUNNING] " << suiteName << std::endl;
        return core::getPerfCounter();
    }

    void endTestGroup(const char* suiteName, i32 returnCode, u64 start) {
        std::cout << "[SUITE " << passedOrFailedStr(returnCode == 0, useAnsiColors) << "] " << suiteName;

        auto end = core::getPerfCounter();
        auto deltaTimeNs = end - start;

        char elapsedTimeBuffer[256];
        std::cout << " [ ";
        std::cout << "time: " << core::testing::elapsedTimeToStr(elapsedTimeBuffer, deltaTimeNs);
        std::cout << " ]";

        std::cout << std::endl;
    }
};
