#pragma once

#include "t-index.h"
#include "test_runner.h"
#include "test_types.h"
#include "surface.h"

void compareFilesBytewise(const char* fileA, const char* fileB);

constexpr const char* SNAPSHOT_ROOT_DIRECTORY = TEST_ASSETS_DIRECTORY "/snapshots";
constexpr const char* TEST_OUTPUT_DIRECTORY = TEST_ASSETS_DIRECTORY "/test_output_directory";

constexpr bool updateAllSnapshots = false;

// Snapshot cases define per-input parameters (name, file, dimensions, update flag, format, origin).
#define SNAPSHOT_CASES(X) \
    X(SIMPLE_TRIANGLE_BGRA8888, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA8888, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGRX8888, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRX8888, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGRA5551, "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGRA5551, BottomLeft) \
    X(SIMPLE_TRIANGLE_BGR555,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR555,   BottomLeft) \
    X(SIMPLE_TRIANGLE_BGR888,   "01 Simple Triangle Scene", "01_simple_triangle.obj", 800, 800, updateAllSnapshots, BGR888,   BottomLeft) \
    X(FOUR_TRIANGLES_BGRA8888,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRA8888, BottomLeft) \
    X(FOUR_TRIANGLES_BGRX8888,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRX8888, BottomLeft) \
    X(FOUR_TRIANGLES_BGRA5551,  "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGRA5551, BottomLeft) \
    X(FOUR_TRIANGLES_BGR555,    "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGR555,   BottomLeft) \
    X(FOUR_TRIANGLES_BGR888,    "02 Four Triangles Scene",  "02_triangles.obj",       800, 800, updateAllSnapshots, BGR888,   BottomLeft)

enum SnapshotIndex {
#define SNAPSHOT_ENUM_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_##id,
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_ENUM_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
#undef SNAPSHOT_ENUM_ENTRY
    SNAPSHOT_COUNT
};

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

inline TestSnapshotInfo testSnapshotInfos[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_INFO_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
};

#define SNAPSHOT_TEST_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin) \
    { \
        .name = case_name, \
        .testFunction = runDirectRasterizationSnapshotTest, \
        .userData = &testSnapshotInfos[SNAPSHOT_##id], \
    },

inline TestCreateInfo snapshotTests[] = {
#define X(id, case_name, file, w, h, upd, fmt, case_origin) SNAPSHOT_TEST_ENTRY(id, case_name, file, w, h, upd, fmt, case_origin)
    SNAPSHOT_CASES(X)
#undef X
};

#undef SNAPSHOT_TEST_ENTRY
#undef SNAPSHOT_INFO_ENTRY
#undef SNAPSHOT_CASES
