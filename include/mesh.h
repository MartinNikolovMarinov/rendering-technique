#pragma once

#include "core_init.h"

struct Mesh3D {
    Mesh3D()
        : m_actx(nullptr)
        , m_vertices({})
        , m_vertCount(0)
        , m_indices({})
        , m_indCount(0) {}

    constexpr inline void setAllocator(core::AllocatorContext& actx) { m_actx = &actx; }
    constexpr bool isOwner() const { return m_actx != nullptr; }
    void free();

    void addVertex(core::vec3f v);

    constexpr i32 vertexCount() { return m_vertCount; }
    constexpr i32 indicesCount() { return m_indCount; }

    constexpr inline core::Memory<core::vec3f> vertices() { return m_vertices.slice(0, addr_size(vertexCount())); }
    constexpr inline core::Memory<core::vec3f> indices() { return m_vertices.slice(0, addr_size(indicesCount())); }

private:
    core::AllocatorContext* m_actx;
    core::Memory<core::vec3f> m_vertices;
    i32 m_vertCount;
    core::Memory<i32> m_indices;
    i32 m_indCount;
};
