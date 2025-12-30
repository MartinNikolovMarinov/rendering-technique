#include "core_init.h"

void assertHandler(const char* failedExpr, const char* file, i32 line, const char* funcName, const char* errMsg) {
    // Using iostream here since assertions can happen inside core as well.

    // Get a stack trace of at max 200 stack frames, skipping the first 2. The first stack frame is this assert handler
    // frame and the second is the function itself, for which we already have information.
    constexpr u32 stackFramesToSkip = 2;
    constexpr addr_size stackTraceBufferSize = core::CORE_KILOBYTE * 8;
    char trace[stackTraceBufferSize] = {};
    addr_size traceLen = 0;
    bool ok = core::stacktrace(trace, stackTraceBufferSize, traceLen, 200, stackFramesToSkip);

    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RED_START() << ANSI_BOLD_START();
    std::cout << "[ASSERTION]:\n  [EXPR]: " << failedExpr
              << "\n  [FUNC]: " << funcName
              << "\n  [FILE]: " << file << ":" << line
              << "\n  [MSG]: " << (errMsg ? errMsg : "");
    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET();

    std::cout << '\n';

    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_BOLD_START();
    std::cout << "[TRACE]:\n" << trace;
    if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET() << std::endl;

    if (!ok) {
        if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RED_START() << ANSI_BOLD_START();
        std::cout << "Failed to take full stacktrace. Consider resizing the stacktrace buffer size!";
        if constexpr (USE_ANSI_LOGGING) std::cout << ANSI_RESET();
        std::cout << std::endl;
    }

    // The only place in the code where an exception is used. Debuggers handle this in a relatively convinient way.
    throw std::runtime_error("Assertion failed!");
}

core::StdStatsAllocator g_statsStdAllocator{};
core::StdAllocator g_stdAllocator{};

core::AllocatorContext createDefautAllocatorCtx() {
#if defined(IS_DEBUG)
    return core::createAllocatorCtx(&g_statsStdAllocator);
#else
    return core::createAllocatorCtx(&g_stdAllocator);
#endif
}

void coreInit(core::LogLevel globalLogLevel) {
    core::LoggerCreateInfo loggerInfo = core::LoggerCreateInfo::createDefault();
    core::loggerSetLevel(globalLogLevel);
    core::initProgramCtx(assertHandler, &loggerInfo, createDefautAllocatorCtx());

    core::registerAllocator(core::createAllocatorCtx(&g_stdAllocator), RA_STD_ALLOCATOR_ID);
    core::registerAllocator(core::createAllocatorCtx(&g_statsStdAllocator), RA_STD_STATS_ALLOCATOR_ID);
}

void coreShutdown() {
    core::destroyProgramCtx(true);
}
