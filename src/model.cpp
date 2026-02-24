#include "model.h"
#include "face.h"

void Model3D::free() {
    if (actx) {
        actx->free(vertices.ptr, vertices.length, sizeof(Vertex4f));
        vertices.clear();

        actx->free(faces.ptr, faces.length, sizeof(Face3i));
        faces.clear();
    }

    *this = {};
}
