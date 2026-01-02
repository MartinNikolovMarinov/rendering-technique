#include "core_init.h"

struct Model3D {
    core::AllocatorContext* actx;

    using Face = i32[3];

    core::Memory<core::vec4f> vertices;
    core::Memory<Face> faces;

    void free();
};
