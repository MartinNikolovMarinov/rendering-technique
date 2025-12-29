#pragma once

#include "core_init.h"

namespace Wavefront {

enum struct WavefrontError {
    Undefined,

    FailedToStatFile,
    FailedToReadFile,

    InvalidFileFormat,

    SENTINEL
};

const char* errorToCstr(WavefrontError err);

struct WavefrontObj {
    struct Object {
        const char* objectName;
        i32 groupsCount;
        i32 groupsOff;
    };

    struct Group {
        const char* groupName;
        i32 facesCount;
        i32 facesOff;
    };

    core::AllocatorContext* actx;
    core::Memory<Object> objects;
    i32 objectsCount;
    core::Memory<Group> groups;
    i32 groupsCount;
    core::Memory<core::vec3f> vertices;
    i32 verticesCount;
    core::Memory<i32> faces;
    i32 facesCount;

    constexpr inline void setAllocator(core::AllocatorContext& _actx) { actx = &_actx; }

    // TODO: does this api make sense ??
    // Get all faces for object and group
    bool nextObj(Object& obj, Group& group, core::Memory<i32>& faces);

    void free();
};

[[nodiscard]] core::expected<WavefrontObj, WavefrontError> loadFile(const char* path, core::AllocatorContext& actx = DEF_ALLOC);

} // Wavefront

