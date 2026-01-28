#include "core_init.h"

struct Face {
    i32 data[3] = {};
    constexpr i32& operator[](i32 idx) { return data[idx]; }
};

struct Model3D {
    core::AllocatorContext* actx;

    core::Memory<core::vec4f> vertices;
    core::Memory<Face> faces;

    void free();
};
