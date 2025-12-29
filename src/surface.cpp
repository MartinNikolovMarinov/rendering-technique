#include "surface.h"

void Surface::free() {
    if (isOwner() && data) {
        actx->free(data, addr_size(size()), sizeof(u8));
    }
}
