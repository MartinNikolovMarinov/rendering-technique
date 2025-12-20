#include "core_init.h"

int main() {
    coreInit(core::LogLevel::L_DEBUG);

    std::cout << (CORE_LOGGING_LEVEL == "DEBUG") << std::endl;

    coreShutdown();
    return 0;
}
