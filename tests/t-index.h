#include "core_init.h"

// ##################### TEST SUITES ###################################################################################

struct TestRunParams;

i32 runWavefrontVerticesTest(const TestRunParams& params);
i32 runWavefrontFacesTest(const TestRunParams& params);

i32 runCreateSurfaceFromTgaFilesInDirectoryTest(const TestRunParams& params);

i32 runDirectRasterizationSnapshotTest(const TestRunParams& params);

i32 runAllTests();
