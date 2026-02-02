#pragma once

#include "t-index.h"
#include "test_runner.h"
#include "test_types.h"
#include "surface.h"

void compareFilesBytewise(const char* fileA, const char* fileB);

constexpr const char* SNAPSHOT_ROOT_DIRECTORY = TEST_ASSETS_DIRECTORY "/snapshots";
constexpr const char* TEST_OUTPUT_DIRECTORY = TEST_ASSETS_DIRECTORY "/test_output_directory";

constexpr bool updateAllSnaps = false;

inline TestSnapshotInfo testSnapshotInfos[] = {
    //==================================================================================================================
    // 01_simple_triangle.obj
    //==================================================================================================================

    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_simple_triangle.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRA8888,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_simple_triangle.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRX8888,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_simple_triangle.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRA5551,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_simple_triangle.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGR555,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/01_simple_triangle.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGR888,
        .origin = Origin::BottomLeft,
    },

    //==================================================================================================================
    // 02_triangles.obj
    //==================================================================================================================

    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/02_triangles.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRA8888,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/02_triangles.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRX8888,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/02_triangles.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGRA5551,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/02_triangles.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGR555,
        .origin = Origin::BottomLeft,
    },
    {
        .wavefrontInputFileFullPath = TEST_ASSETS_DIRECTORY "/snapshot_tests_input_files/02_triangles.obj",
        .snapshotDirectory = SNAPSHOT_ROOT_DIRECTORY,
        .outputDirectory = TEST_OUTPUT_DIRECTORY,
        .updateSnapshots = updateAllSnaps,
        .width = 800,
        .height = 800,
        .pixelFormat = PixelFormat::BGR888,
        .origin = Origin::BottomLeft,
    },
};

inline TestCreateInfo snapshotTests[] = {
    //==================================================================================================================
    // 01_simple_triangle.obj
    //==================================================================================================================

    {
        .name = "01 Simple Triangle Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[0]
    },
    {
        .name = "01 Simple Triangle Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[1]
    },
    {
        .name = "01 Simple Triangle Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[2]
    },
    {
        .name = "01 Simple Triangle Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[3]
    },
    {
        .name = "01 Simple Triangle Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[4]
    },

    //==================================================================================================================
    // 02_triangles.obj
    //==================================================================================================================

    {
        .name = "02 Four Triangles Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[5]
    },
    {
        .name = "02 Four Triangles Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[6]
    },
    {
        .name = "02 Four Triangles Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[7]
    },
    {
        .name = "02 Four Triangles Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[8]
    },
    {
        .name = "02 Four Triangles Scene",
        .testFunction = runDirectRasterizationSnapshotTest,
        .userData = &testSnapshotInfos[9]
    },
};
