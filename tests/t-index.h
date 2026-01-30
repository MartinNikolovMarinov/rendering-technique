#include "core_init.h"

// ##################### TEST SUITES ###################################################################################

struct TestRunParams;

// TODO: Is this the right place for this?
struct TestSnapshotInfo {
    const char* wavefrontInputFile = nullptr;
    const char* snapshotDirectory = nullptr;
    bool updateSnapshots = false;
};

i32 runWavefrontTestsSuite(const TestRunParams& params);
i32 runTgaTestsSuite(const TestRunParams& params);
i32 runRenderSingleCenteredTriangle(const TestRunParams& params);

i32 runAllTests();
