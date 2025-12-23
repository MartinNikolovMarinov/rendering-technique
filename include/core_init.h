#pragma once

#include <core.h>
#include <core_extensions/hash_functions.h>

using namespace coretypes;

#define DEF_ALLOC core::getAllocator(core::DEFAULT_ALLOCATOR_ID)

void coreInit(core::LogLevel globalLogLevel);
void coreShutdown();
