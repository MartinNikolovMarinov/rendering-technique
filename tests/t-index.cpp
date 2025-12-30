#include "t-index.h"

namespace {

core::AllocatorId allAllocatorsArr[RegisteredAllocators::RA_SENTINEL - 1];
auto allAllocators = core::Memory<const core::AllocatorId>(allAllocatorsArr, RegisteredAllocators::RA_SENTINEL - 1);

template <typename TSuite>
i32 runWithNAllocators(core::testing::TestSuiteInfo& sInfo, core::Memory<const core::AllocatorId> allocatorIds, TSuite suite) {
    using namespace core::testing;

    for (addr_size i = 0; i < allocatorIds.len(); i++) {
        auto aid = allocatorIds[i];
        sInfo.actx = &core::getAllocator(aid);
        if (runTestSuite(sInfo, suite) != 0) {
            return -1;
        }
    }

    return 0;
}

} // namespace

i32 runAllTests() {
    using namespace core::testing;

    coreInit(core::LogLevel::L_DEBUG);
    defer { coreShutdown(); };

    // Set all allocators array
    for (auto i = RegisteredAllocators::RA_DEFAULT + 1; i < RegisteredAllocators::RA_SENTINEL; i++) {
        allAllocatorsArr[i-1] = i;
    }

    i32 ret = 0;

    TestSuiteInfo sInfo;
    sInfo.useAnsiColors = true;
    sInfo.trackTime = true;

    sInfo.name = FN_NAME_TO_CPTR(runWavefrontTestsSuite);
    if (runWithNAllocators(sInfo, allAllocators, runWavefrontTestsSuite) != 0) { ret = -1; }

    return ret;
}
