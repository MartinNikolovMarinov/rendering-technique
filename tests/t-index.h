#include "core_init.h"

// ##################### TEST SUITES ###################################################################################

struct TestRunParams;

i32 runWavefrontVerticesTest(TestRunParams& params);
i32 runWavefrontFacesTest(TestRunParams& params);

i32 runCreateSurfaceFromTgaFilesInDirectoryTest(TestRunParams& params);

i32 runDirectRasterizationSnapshotTest(TestRunParams& params);
i32 runTriangleInsideTriangleDirectDrawingSnapshotTest(TestRunParams& params);

i32 runAllTests();
