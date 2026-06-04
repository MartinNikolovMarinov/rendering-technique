#pragma once

#include "core_init.h" // IWYU pragma: keep

struct Surface;

[[nodiscard]] bool initializeDebugRendering();
void shutdownDebugRendering();

void debug_immPreviewSurface(const Surface& surface);
