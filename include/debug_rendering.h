#include "core_init.h"

struct Surface;

bool initializeDebugRendering();
void shutdownDebugRendering();

void debug_immPreviewSurface(const Surface& surface);
