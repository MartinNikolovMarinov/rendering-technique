#pragma once

#include "core_init.h"
#include "surface.h"

struct TestSnapshotInfo {
    const char* wavefrontInputFileFullPath = nullptr;
    const char* snapshotDirectory = nullptr;
    const char* outputDirectory = nullptr;
    bool updateSnapshots = false;

    i32 width = 800;
    i32 height = 800;
    PixelFormat pixelFormat = PixelFormat::BGRA8888;
    Origin origin = Origin::BottomLeft;
};
