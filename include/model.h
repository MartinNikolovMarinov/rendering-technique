#pragma once

#include "core_init.h"

using Vertex4f = core::vec4f;
struct Face3i;

struct Model3D {
    core::AllocatorContext* actx;

    core::Memory<Vertex4f> vertices;
    core::Memory<Face3i> faces;

    void free();
};
