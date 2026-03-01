#pragma once

#include "core_init.h"

struct Face3i;

struct Model3D {
    core::AllocatorContext* actx;

    core::Memory<Vertex4f> vertices;
    core::Memory<Face3i> faces;

    void free();
};
