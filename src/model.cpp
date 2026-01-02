#include "model.h"

void Model3D::free() {
    if (actx) {
        core::memoryFree(std::move(vertices), *actx);
        core::memoryFree(std::move(faces), *actx);
    }

    *this = {};
}
