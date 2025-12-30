#include "wavefront_files.h"
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

[[nodiscard]] bool hasNextComponent(core::StrView line);
[[nodiscard]] core::expected<f32, WavefrontError> parseFloatComponent(
    core::StrView& component,
    core::StrView& currLine,
    char delim);

[[nodiscard]] core::expected<core::vec4f, WavefrontError> parseVertexLine(core::StrView currLine);

} // namespace

const char* errorToCstr(WavefrontError err) {
    switch (err) {
        case WavefrontError::UnsupportedVersion: return "Unsupported Wavefront file version";
        case WavefrontError::FailedToStatFile:   return "Failed to stat file";
        case WavefrontError::FailedToReadFile:   return "Failed to read file";
        case WavefrontError::InvalidFileFormat:  return "Invalid file format";

        case WavefrontError::Undefined: [[fallthrough]];
        case WavefrontError::SENTINEL:  return "unknown";
    }
}

void WavefrontObj::free() {
    if (actx) {
        core::memoryFree(std::move(vertices), *actx);
        core::memoryFree(std::move(faces), *actx);
    }

    verticesCount = 0;
    facesCount = 0;
}

core::expected<WavefrontObj, WavefrontError> loadFile(const char* path,
                                                      WavefrontVersion fileVersion,
                                                      core::AllocatorContext& actx
) {
    if (fileVersion != WavefrontVersion::VERSION_3_0) {
        return core::unexpected(WavefrontError::UnsupportedVersion);
    }

    core::FileStat fileStat;
    auto statRes = core::fileStat(path, fileStat);
    WAVEFRONT_PLT_ERR_CHECK(statRes, WavefrontError::FailedToStatFile);

    addr_size fsize = fileStat.size;
    auto fileMemoryRaw = core::memoryZeroAllocate<u8>(fsize, actx);
    defer { core::memoryFree(std::move(fileMemoryRaw), actx); };

    auto readEntireRes = core::fileReadEntire(path, fileMemoryRaw);
    WAVEFRONT_PLT_ERR_CHECK(readEntireRes, WavefrontError::FailedToReadFile);

    WavefrontObj obj = {};
    obj.actx = &actx;

    core::StrView rest = core::sv(fileMemoryRaw);
    core::StrView currLine;

    while (!rest.empty()) {
        rest = core::cut(rest, '\n', currLine, true);

        if(currLine.empty()) continue;

        if (core::startsWith(currLine, "v ")) {
            // vertex
            auto res = parseVertexLine(currLine);
            if (res.hasErr()) return core::unexpected(res.err());

            obj.vertices = core::memorySet(obj.vertices, addr_size(obj.verticesCount), std::move(res.value()), *obj.actx);
            obj.verticesCount++;
        }
        else if (core::startsWith(currLine, "f ")) {
            // TODO: parse faces
        }
    }

    return obj;
}

namespace {

bool hasNextComponent(core::StrView line) {
    core::StrView trimmedLine = core::trim(line);
    for (addr_size i = 0; i < trimmedLine.len(); i++) {
        if (core::isWhiteSpace(trimmedLine[i])) {
            return true;
        }
    }
    return false;
}

core::expected<f32, WavefrontError> parseFloatComponent(
    core::StrView& component,
    core::StrView& currLine,
    char delim
) {
    currLine = core::cut(currLine, delim, component);
    currLine = core::trimWhiteSpaceLeft(currLine);
    component = core::trim(component);
    auto x = core::cstrToFloat<f32>(component.data(), u32(component.len()));
    WAVEFRONT_CONV_ERR_CHECK(x);
    return f32(x.value());
}

core::expected<core::vec4f, WavefrontError> parseVertexLine(core::StrView currLine) {
    Assert(currLine[0] == 'v', "BUG: failed a basic sanity check");

    core::vec4f vertex;
    core::StrView component;

#if defined(IS_DEBUG)
    vertex = core::v(-99.0f, -99.0f, -99.0f, -99.0f);
#endif

    // Skip the first component which describes the type of object.
    currLine = core::cut(currLine, ' ', component);
    currLine = core::trimWhiteSpaceLeft(currLine);

    // Parse X component
    auto x = parseFloatComponent(component, currLine, ' ');
    if (x.hasErr()) return core::unexpected(x.err());
    vertex.x() = x.value();

    // Parse Y component
    auto y = parseFloatComponent(component, currLine, ' ');
    if (y.hasErr()) return core::unexpected(y.err());
    vertex.y() = y.value();

    // Parse Z component
    bool hasMoreComponents = hasNextComponent(currLine);
    auto z = parseFloatComponent(component, currLine, hasMoreComponents ? ' ' : '\n');
    if (z.hasErr()) return core::unexpected(z.err());
    vertex.z() = z.value();

    // Parse optional W component
    if (hasMoreComponents) {
        auto w = parseFloatComponent(component, currLine, '\n');
        if (w.hasErr()) return core::unexpected(w.err());
        vertex.w() = w.value();
    }

    return vertex;
}

} // namespace

} // Wavefront
