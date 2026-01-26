#include "core_init.h"

struct Model3D {
    core::AllocatorContext* actx;

    struct Face {
        i32 data[3] = {};
        constexpr i32& operator[](i32 idx) { return data[idx]; }
    };

    core::Memory<core::vec4f> vertices;
    core::Memory<Face> faces;

    void free();
};
