#include "core_init.h"

// ##################### TEST SUITES ###################################################################################

struct TestRunParams;

struct TestSnapshotInfo {
    const char* wavefrontInputFileFullPath = nullptr;
    const char* snapshotDirectory = nullptr;
    const char* outputDirectory = nullptr;
    bool updateSnapshots = false;
};

i32 runWavefrontVerticesTest(const TestRunParams& params);
i32 runWavefrontFacesTest(const TestRunParams& params);

i32 runCreateSurfaceFromTgaFilesInDirectoryTest(const TestRunParams& params);

i32 runDirectRasterizationSnapshotTest(const TestRunParams& params);

i32 runAllTests();
