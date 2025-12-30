#include "t-index.h"

#include <iostream>

i32 main() {
    std::cout << "[CORE VERSION] "
              << CORE_VERSION_MAJOR << "."
              << CORE_VERSION_MINOR << "."
              << CORE_VERSION_PATCH
              << std::endl;

    // Print compiler
    if constexpr (COMPILER_CLANG == 1)   { std::cout << "[COMPILER] COMPILER_CLANG" << std::endl; }
    if constexpr (COMPILER_GCC == 1)     { std::cout << "[COMPILER] COMPILER_GCC" << std::endl; }
    if constexpr (COMPILER_MSVC == 1)    { std::cout << "[COMPILER] COMPILER_MSVC" << std::endl; }
    if constexpr (COMPILER_UNKNOWN == 1) { std::cout << "[COMPILER] COMPILER_UNKNOWN" << std::endl; }

    // Print OS
    if constexpr (OS_WIN == 1)     { std::cout << "[OS] OS_WIN" << std::endl; }
    if constexpr (OS_LINUX == 1)   { std::cout << "[OS] OS_LINUX" << std::endl; }
    if constexpr (OS_MAC == 1)     { std::cout << "[OS] OS_MAC" << std::endl; }
    if constexpr (OS_UNKNOWN == 1) { std::cout << "[OS] OS_UNKNOWN" << std::endl; }

    if constexpr (CORE_DEBUG == 1) {
        std::cout << "[MODE] DEBUG"  << std::endl;
    }
    else {
        std::cout << "[MODE] RELEASE" << std::endl;
    }

    i32 exitCode = runAllTests();

    return exitCode;
}
