#pragma once

#include "core_init.h"

struct Mesh3D;

namespace Wavefront {

enum struct WavefrontError {
    Undefined,

    FailedToStatFile,
    FailedToReadFile,

    InvalidFileFormat,

    SENTINEL
};

const char* errorToCstr(WavefrontError err);

[[nodiscard]] core::expected<Mesh3D, WavefrontError> loadMesh(const char* path, core::AllocatorContext& actx = DEF_ALLOC);

} // Wavefront

