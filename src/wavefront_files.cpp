#include "wavefront_files.h"
#include "mesh.h"
#include "log_utils.h"

#define WAVEFRONT_CONV_ERR_CHECK(x) \
    if (x.hasErr()) { \
        logErr_ConvErrorCode(x.err()); \
        return core::unexpected(Wavefront::WavefrontError::InvalidFileFormat); \
    }

#define WAVEFRONT_PLT_ERR_CHECK(x, errType) \
    if (x.hasErr()) { \
        logErr_PltErrorCode(x.err()); \
        return core::unexpected(errType); \
    }

namespace Wavefront {

namespace {

[[nodiscard]] core::expected<core::vec3f, WavefrontError> parseVertex(core::StrView currentLine);

} // namespace

const char* errorToCstr(WavefrontError err) {
    switch (err) {
        case WavefrontError::FailedToStatFile:  return "Failed to stat file";
        case WavefrontError::FailedToReadFile:  return "Failed to read file";
        case WavefrontError::InvalidFileFormat: return "Invalid file format";

        case WavefrontError::Undefined: [[fallthrough]];
        case WavefrontError::SENTINEL:  return "unknown";
    }
}

core::expected<Mesh3D, WavefrontError> loadMesh(const char* path, core::AllocatorContext& actx) {
    core::FileStat fileStat;
    auto statRes = core::fileStat(path, fileStat);
    WAVEFRONT_PLT_ERR_CHECK(statRes, WavefrontError::FailedToStatFile);

    addr_size fsize = fileStat.size;
    auto fileMemoryRaw = core::memoryZeroAllocate<u8>(fsize, actx);
    defer { core::memoryFree(std::move(fileMemoryRaw), actx); };

    auto readEntireRes = core::fileReadEntire(path, fileMemoryRaw);
    WAVEFRONT_PLT_ERR_CHECK(readEntireRes, WavefrontError::FailedToReadFile);

    Mesh3D mesh;
    mesh.setAllocator(actx);

    core::StrView rest = core::sv(fileMemoryRaw);
    core::StrView currLine;

    while (!rest.empty()) {
        rest = core::cut(rest, '\n', currLine, true);

        if(currLine.empty()) continue;

        if (core::startsWith(currLine, "v ")) {
            // vertex
            auto res = parseVertex(currLine);
            if (res.hasErr()) return core::unexpected(res.err());
            mesh.addVertex(res.value());
        }
        else if (core::startsWith(currLine, "f ")) {
            // TODO: parse faces
        }
    }

    return mesh;
}

namespace {

core::expected<core::vec3f, WavefrontError> parseVertex(core::StrView currLine) {
    Assert(currLine[0] == 'v', "BUG: failed a basic sanity check");

    core::vec3f vertex;
    core::StrView component;

#if defined(IS_DEBUG)
    vertex = core::v(-99.0f, -99.0f, -99.0f);
#endif

    // Skip the first component which describes the type of object.
    currLine = core::cut(currLine, ' ', component);
    currLine = core::trimWhiteSpaceLeft(currLine);

    // Parse X component
    {
        currLine = core::cut(currLine, ' ', component);
        currLine = core::trimWhiteSpaceLeft(currLine);
        component = core::trim(component);
        auto x = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(x);
        vertex.x() = x.value();
    }

    // Parse Y component
    {
        currLine = core::cut(currLine, ' ', component);
        currLine = core::trimWhiteSpaceLeft(currLine);
        component = core::trim(component);
        auto y = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(y);
        vertex.y() = y.value();
    }

    // Parse Z component
    {
        currLine = core::cut(currLine, '\n', component);
        component = core::trim(component);
        auto z = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(z);
        vertex.z() = z.value();
    }

    return vertex;
}

} // namespace

} // Wavefront
