#pragma once

#include "t-index.h"
#include "test_runner.h"
#include "test_types.h"
#include "surface.h"

constexpr const char* SNAPSHOT_ROOT_DIRECTORY = TEST_ASSETS_DIRECTORY "/snapshots";
constexpr const char* TEST_OUTPUT_DIRECTORY = TEST_ASSETS_DIRECTORY "/test_output_directory";

constexpr bool updateAllSnapshots = false;

// Snapshot cases are the single source of truth for snapshot tests.
// Each line defines one test variant:
//   id, display name, input file, width, height, update flag, pixel format, origin.
// The id becomes an enum entry (SNAPSHOT_<id>) so both tables index the same element.
// Editing this list updates both TestSnapshotInfo and TestCreateInfo tables below.
#define SNAPSHOT_SIMPLE_TRIANGLE_SCENE(X) \
    X(SIMPLE_TRIANGLE_BGRA8888_300x300, "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, BGRA8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX8888_300x300, "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, BGRX8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRA5551_300x300, "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, BGRA5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGR555_300x300,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, BGRX5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGR888_300x300,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, BGR888,   BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAY8_300x300,    "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, GRAY8,    BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAY88_300x300,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 300, 300, updateAllSnapshots, GRAYA88,  BottomLeft, runDirectRasterizationSnapshotTest) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888_437x658, "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, BGRA8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX8888_437x658, "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, BGRX8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRA5551_437x658, "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, BGRA5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX5551_437x658, "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, BGRX5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGR888_437x658,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, BGR888,   BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAY8_437x658,    "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, GRAY8,    BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAYA88_437x658,  "01 Simple Triangle Scene", "01_simple_triangle.obj", 437, 658, updateAllSnapshots, GRAYA88,  BottomLeft, runDirectRasterizationSnapshotTest) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888_10x800, "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, BGRA8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX8888_10x800, "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, BGRX8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRA5551_10x800, "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, BGRA5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX5551_10x800, "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, BGRX5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGR888_10x800,   "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, BGR888,   BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAY8_10x800,    "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, GRAY8,    BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAYA88_10x800,  "01 Simple Triangle Scene", "01_simple_triangle.obj",  40, 800, updateAllSnapshots, GRAYA88,  BottomLeft, runDirectRasterizationSnapshotTest) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888_1000x20, "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, BGRA8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX8888_1000x20, "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, BGRX8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRA5551_1000x20, "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, BGRA5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGRX5551_1000x20, "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, BGRX5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_BGR888_1000x20,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, BGR888,   BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAY8_1000x20,    "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, GRAY8,    BottomLeft, runDirectRasterizationSnapshotTest) \
    X(SIMPLE_TRIANGLE_GRAYA88_1000x20,  "01 Simple Triangle Scene", "01_simple_triangle.obj", 1000, 60, updateAllSnapshots, GRAYA88,  BottomLeft, runDirectRasterizationSnapshotTest) \
    \

#define SNAPSHOT_TRIANGLE_SCENE(X) \
    X(FOUR_TRIANGLES_BGRA8888_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, BGRA8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX8888_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, BGRX8888, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRA5551_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, BGRA5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX5551_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, BGRX5551, BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGR888_BottomLeft,    "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, BGR888,   BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAY8_BottomLeft,     "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, GRAY8,    BottomLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAYA88_BottomLeft,   "02 Four Triangles Scene",  "02_triangles.obj",  800, 800, updateAllSnapshots, GRAYA88,  BottomLeft, runDirectRasterizationSnapshotTest) \
    \
    X(FOUR_TRIANGLES_BGRA8888_TopLeft,  "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, BGRA8888, TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX8888_TopLeft,  "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, BGRX8888, TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRA5551_TopLeft,  "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, BGRA5551, TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX5551_TopLeft,  "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, BGRX5551, TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGR888_TopLeft,    "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, BGR888,   TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAY8_TopLeft,     "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, GRAY8,    TopLeft, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAYA88_TopLeft,   "02 Four Triangles Scene",  "02_triangles.obj",     800, 800, updateAllSnapshots, GRAYA88,  TopLeft, runDirectRasterizationSnapshotTest) \
    \
    X(FOUR_TRIANGLES_BGRA8888_BottomRight,  "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, BGRA8888, BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX8888_BottomRight,  "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, BGRX8888, BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRA5551_BottomRight,  "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, BGRA5551, BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX5551_BottomRight,  "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, BGRX5551, BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGR888_BottomRight,    "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, BGR888,   BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAY8_BottomRight,     "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, GRAY8,    BottomRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAYA88_BottomRight,   "02 Four Triangles Scene",  "02_triangles.obj", 800, 800, updateAllSnapshots, GRAYA88,  BottomRight, runDirectRasterizationSnapshotTest) \
    \
    X(FOUR_TRIANGLES_BGRA8888_TopRight,  "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, BGRA8888, TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX8888_TopRight,  "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, BGRX8888, TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRA5551_TopRight,  "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, BGRA5551, TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGRX5551_TopRight,  "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, BGRX5551, TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_BGR888_TopRight,    "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, BGR888,   TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAY8_TopRight,     "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, GRAY8,    TopRight, runDirectRasterizationSnapshotTest) \
    X(FOUR_TRIANGLES_GRAYA88_TopRight,   "02 Four Triangles Scene",  "02_triangles.obj",    800, 800, updateAllSnapshots, GRAYA88,  TopRight, runDirectRasterizationSnapshotTest) \
    \

#define SNAPSHOT_CASES(X) \
    SNAPSHOT_SIMPLE_TRIANGLE_SCENE(X) \
    SNAPSHOT_TRIANGLE_SCENE(X)

// Maps each case id to a stable index used to cross-reference both tables.
// The INFO/TEST entry macros expand SNAPSHOT_CASES into parallel arrays:
//   - testSnapshotInfos: data needed by the snapshot runner
//   - snapshotTests:     test registration pointing at the matching info entry
#define SNAPSHOT_ENUM_ENTRY(id) SNAPSHOT_##id,
#define SNAPSHOT_INFO_ENTRY(file, w, h, upd, fmt, case_origin) \
    { \
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/" file, \
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY, \
        .outputDirectory = TEST_OUTPUT_DIRECTORY, \
        .updateSnapshots = upd, \
        .width = w, \
        .height = h, \
        .pixelFormat = PixelFormat::fmt, \
        .origin = Origin::case_origin, \
    },
#define SNAPSHOT_TEST_ENTRY(id, case_name, test_fn) \
    { \
        .name = case_name, \
        .testFunction = test_fn, \
        .userData = &testSnapshotInfos[SNAPSHOT_##id], \
    },

enum SnapshotIndex {
#define X(id, case_name, file, w, h, upd, fmt, case_origin, test_fn) SNAPSHOT_ENUM_ENTRY(id)
    SNAPSHOT_CASES(X)
#undef X
    SNAPSHOT_COUNT
};

inline TestSnapshotInfo testSnapshotInfos[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin, test_fn) SNAPSHOT_INFO_ENTRY(file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
};

inline TestCreateInfo snapshotTests[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin, test_fn) SNAPSHOT_TEST_ENTRY(id, case_name, test_fn)
    SNAPSHOT_CASES(X)
#undef X
};

#undef SNAPSHOT_TEST_ENTRY
#undef SNAPSHOT_INFO_ENTRY
#undef SNAPSHOT_CASES
#undef SNAPSHOT_ENUM_ENTRY
