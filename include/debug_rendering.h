#pragma once

#include "core_init.h"

struct Surface;

[[nodiscard]] bool initializeDebugRendering();
void shutdownDebugRendering();

void debug_immPreviewSurface(const Surface& surface);
