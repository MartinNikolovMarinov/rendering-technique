#include "wavefront_files.h"
#include "log_utils.h"

// TODO: [WAVEFRONT] This code expects spaces if a wavefront file with tabs for delimiters is ever passed it will fail.

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

template <i32 N>
[[nodiscard]] core::StrView nextToken(core::StrView line, const char (&delims)[N], core::StrView& token);
[[nodiscard]] core::StrView skipToken(core::StrView line, char delim);
[[nodiscard]] i32 countTokens(core::StrView currLine, char delim);

[[nodiscard]] core::expected<core::vec4f, WavefrontError> parseVertexLine(core::StrView currLine);
[[nodiscard]] core::expected<WavefrontObj::Face, WavefrontError> parseFaces(core::StrView currLine);

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

    *this = {};
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
            // faces
            auto res = parseFaces(currLine);
            if (res.hasErr()) return core::unexpected(res.err());

            WavefrontObj::Face face = std::move(res.value());
            obj.faces = core::memorySet(obj.faces, addr_size(obj.facesCount), std::move(face), *obj.actx);
            obj.facesCount++;
        }
    }

    return obj;
}

namespace {

template <i32 N>
core::StrView nextToken(core::StrView line, const char (&delims)[N], core::StrView& token) {
    token = {};
    core::StrView ret = {};
    for (i32 i = 0; i < N; i++) {
        core::StrView component = {};
        core::StrView rest = core::cut(line, delims[i], component);
        if (!component.empty()) {
            token = core::trim(component);
            ret = core::trimWhiteSpaceLeft(rest);
            break;
        }
    }

    return ret;
}

core::StrView skipToken(core::StrView line, char delim) {
    [[maybe_unused]] core::StrView unused;
    auto ret = nextToken(line, { delim }, unused);
    return ret;
}

i32 countTokens(core::StrView currLine, char delim) {
    i32 count = 0;
    while (true) {
        currLine = skipToken(currLine, delim);
        if (currLine.empty()) break;
        count++;
    }

    return count;
}

core::expected<core::vec4f, WavefrontError> parseVertexLine(core::StrView currLine) {
    Assert(currLine[0] == 'v', "BUG: failed a basic sanity check");

    core::vec4f vertex;
    core::StrView component;

#if defined(IS_DEBUG)
    vertex = core::v(-99.0f, -99.0f, -99.0f, -99.0f);
#endif

    // Skip 'v '
    currLine = skipToken(currLine, ' ');

    // Parse X component
    {
        currLine = nextToken(currLine, { ' ' }, component);
        auto x = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(x);
        vertex.x() = f32(x.value());
    }

    // Parse Y component
    {
        currLine = nextToken(currLine, { ' ' }, component);
        auto y = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(y);
        vertex.y() = f32(y.value());
    }

    // Parse Z component
    {
        currLine = nextToken(currLine, { ' ', '\n' }, component);
        auto z = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(z);
        vertex.z() = f32(z.value());
    }

    // Parse optional W component
    if (!currLine.empty()) {
        currLine = nextToken(currLine, { ' ', '\n' }, component);
        auto w = core::cstrToFloat<f32>(component.data(), u32(component.len()));
        WAVEFRONT_CONV_ERR_CHECK(w);
        vertex.w() = f32(w.value());
    }

    return vertex;
}

[[nodiscard]] core::expected<WavefrontObj::Face, WavefrontError> parseFaces(core::StrView currLine) {
    using WavefrontObj = WavefrontObj;
    using Face = WavefrontObj::Face;
    constexpr i32 DIMMENTIONS = WavefrontObj::Face::DIMMENTIONS;

    Assert(currLine[0] == 'f', "BUG: failed a basic sanity check");

    auto parseFaceComponent = [](core::StrView component, Face& face, i32 faceIdx) -> core::expected<WavefrontError> {
        core::StrView faceComponents[DIMMENTIONS];
        addr_size faceComponentsCount = 0;
        bool ok = core::split(component, '/', faceComponents, DIMMENTIONS, faceComponentsCount);
        if (!ok) {
            return core::unexpected(WavefrontError::InvalidFileFormat);
        }

        for (addr_size i = 0; i < faceComponentsCount; i++) {
            auto fcSv = faceComponents[i];
            if (fcSv.len() > 0) {
                auto res = core::cstrToInt<i32>(fcSv.data(), u32(fcSv.len()));
                WAVEFRONT_CONV_ERR_CHECK(res);
                face.data[i][faceIdx] = res.value();
                face.set(i32(i), faceIdx);
            }
        }

        return {};
    };

    WavefrontObj::Face face = {};

    // Skip 'f '
    currLine = skipToken(currLine, ' ');

    i32 componentsCount = countTokens(currLine, ' ');

    if (componentsCount != DIMMENTIONS - 1) {
        logErr(
            "TODO: [WAVEFRONT] Face components with more than {} dimensions are not supported yet; or maybe never will.",
            DIMMENTIONS
        );
        return core::unexpected(WavefrontError::InvalidFileFormat);
    }

    // Parse vertex indices
    {
        core::StrView component = {};
        currLine = nextToken(currLine, { ' ' }, component);
        if (component.empty()) {
            return core::unexpected(WavefrontError::InvalidFileFormat);
        }
        auto res = parseFaceComponent(component, face, 0);
        if (res.hasErr()) return core::unexpected(res.err());
        componentsCount--;
    }

    // Parse vertex texture indices
    {
        core::StrView component = {};
        currLine = nextToken(currLine, { ' ' }, component);
        if (component.empty()) {
            return core::unexpected(WavefrontError::InvalidFileFormat);
        }
        auto res = parseFaceComponent(component, face, 1);
        if (res.hasErr()) return core::unexpected(res.err());
        componentsCount--;
    }

    // Parse vertex normal indices
    {
        core::StrView component = {};
        currLine = nextToken(currLine, { ' ', '\n' }, component);
        if (component.empty()) {
            return core::unexpected(WavefrontError::InvalidFileFormat);
        }
        auto res = parseFaceComponent(component, face, 2);
        if (res.hasErr()) return core::unexpected(res.err());
        componentsCount--;
    }

    return face;
}

} // namespace

} // Wavefront
