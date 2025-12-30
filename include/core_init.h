#pragma once

#include <core.h>
#include <core_extensions/hash_functions.h>

using namespace coretypes;

enum RegisteredAllocators : core::AllocatorId {
    RA_DEFAULT,

    RA_STD_ALLOCATOR_ID,
    RA_STD_STATS_ALLOCATOR_ID,

    RA_SENTINEL
};

#define DEF_ALLOC core::getAllocator(core::DEFAULT_ALLOCATOR_ID)

void coreInit(core::LogLevel globalLogLevel);
void coreShutdown();
