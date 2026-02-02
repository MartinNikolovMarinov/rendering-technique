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
#define SNAPSHOT_CASES(X) \
    X(SIMPLE_TRIANGLE_BGRA8888_BottomLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA8888, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGRX8888_BottomLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRX8888, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGRA5551_BottomLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA5551, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGR555_BottomLeft,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR555,   BottomLeft) \
    X(SIMPLE_TRIANGLE_BGR888_BottomLeft,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR888,   BottomLeft) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA8888, TopLeft) \
    X(SIMPLE_TRIANGLE_BGRX8888, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRX8888, TopLeft) \
    X(SIMPLE_TRIANGLE_BGRA5551, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA5551, TopLeft) \
    X(SIMPLE_TRIANGLE_BGR555,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR555,   TopLeft) \
    X(SIMPLE_TRIANGLE_BGR888,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR888,   TopLeft) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888_BottomRight, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA8888, BottomRight) \
    X(SIMPLE_TRIANGLE_BGRX8888_BottomRight, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRX8888, BottomRight) \
    X(SIMPLE_TRIANGLE_BGRA5551_BottomRight, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA5551, BottomRight) \
    X(SIMPLE_TRIANGLE_BGR555_BottomRight,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR555,   BottomRight) \
    X(SIMPLE_TRIANGLE_BGR888_BottomRight,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR888,   BottomRight) \
    \
    X(SIMPLE_TRIANGLE_BGRA8888_TopLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA8888, TopLeft) \
    X(SIMPLE_TRIANGLE_BGRX8888_TopLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRX8888, TopLeft) \
    X(SIMPLE_TRIANGLE_BGRA5551_TopLeft, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA5551, TopLeft) \
    X(SIMPLE_TRIANGLE_BGR555_TopLeft,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR555,   TopLeft) \
    X(SIMPLE_TRIANGLE_BGR888_TopLeft,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR888,   TopLeft) \
    \
    X(FOUR_TRIANGLES_BGRA8888_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRA8888, BottomLeft) \
    X(FOUR_TRIANGLES_BGRX8888_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRX8888, BottomLeft) \
    X(FOUR_TRIANGLES_BGRA5551_BottomLeft,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRA5551, BottomLeft) \
    X(FOUR_TRIANGLES_BGR555_BottomLeft,    "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGR555,   BottomLeft) \
    X(FOUR_TRIANGLES_BGR888_BottomLeft,    "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGR888,   BottomLeft)

// Maps each case id to a stable index used to cross-reference both tables.
// The INFO/TEST entry macros expand SNAPSHOT_CASES into parallel arrays:
//   - testSnapshotInfos: data needed by the snapshot runner
//   - snapshotTests:     test registration pointing at the matching info entry
#define SNAPSHOT_ENUM_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_##id,
#define SNAPSHOT_INFO_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin) \
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
#define SNAPSHOT_TEST_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin) \
    { \
        .name = case_name, \
        .testFunction = runDirectRasterizationSnapshotTest, \
        .userData = &testSnapshotInfos[SNAPSHOT_##id], \
    },

enum SnapshotIndex {
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_ENUM_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
    SNAPSHOT_COUNT
};

inline TestSnapshotInfo testSnapshotInfos[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_INFO_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
};

inline TestCreateInfo snapshotTests[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_TEST_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
};

#undef SNAPSHOT_TEST_ENTRY
#undef SNAPSHOT_INFO_ENTRY
#undef SNAPSHOT_CASES
#undef SNAPSHOT_ENUM_ENTRY
