#include "mesh.h"

void Mesh3D::free() {
    if (isOwner()) {
        if (!m_vertices.empty()) {
            core::memoryFree(std::move(m_vertices), *m_actx);
        }
        if (!m_indices.empty()) {
            core::memoryFree(std::move(m_indices), *m_actx);
        }
    }
}

void Mesh3D::addVertex(core::vec3f v) {
    Assert(isOwner());
    m_vertices = core::memorySet(m_vertices, addr_size(m_vertCount), std::move(v), *m_actx);
    m_vertCount++;
}
