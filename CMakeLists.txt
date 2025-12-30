cmake_minimum_required(VERSION 3.5)
project(rendering-techniques VERSION 0.0.1 LANGUAGES C CXX)

set(target_sandbox sandbox)
set(target_test tests)

# Standard Requirements:

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Add directories from lib cmake projects

option(CORE_LIBRARY_SHARED "Build core as a shared library." OFF)
option(CORE_ASSERT_ENABLED "Enable Asserts in corelib" OFF)
add_subdirectory(lib/core)

# Include cmake modules from:

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_local")

include(CompilerOptions)
include(Logger)
include(RenderingTechniquesClangOptions)

init_logger("[RENDERING_TECHNIQUES]")

# ---------------------------------------- Begin Options ---------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RENDERING_TECHNIQUES_DEBUG OFF)
else()
    set(RENDERING_TECHNIQUES_DEBUG ON)
endif()

option(USE_ANSI_LOGGING "Use ANSI escape codes in logging." OFF)
option(HOT_SWAP "Build with hotswapping functionality." OFF)

# Print Selected Options:

log_info("---------------------------------------------")
log_info("Version:           ${PROJECT_VERSION}")
log_info("Platform:          ${OS}")
log_info("Arch:              ${CMAKE_SYSTEM_PROCESSOR}")
log_info("C++ Version:       ${CMAKE_CXX_STANDARD}")
log_info("Compiler:          ${CMAKE_CXX_COMPILER_ID}")
log_info("Debug:             ${RENDERING_TECHNIQUES_DEBUG}")
log_info("Hotswap:           ${HOT_SWAP}")
log_info("Use ANSI logging:  ${USE_ANSI_LOGGING}")
log_info("---------------------------------------------")

# ---------------------------------------- End Options -----------------------------------------------------------------

# ---------------------------------------- Begin Declare Source Files --------------------------------------------------

set(src_common
    src/core_init.cpp
    src/tga_files.cpp
    src/log_utils.cpp
    src/surface.cpp
    src/debug_rendering_glfw.cpp
    src/surface_renderer.cpp
    src/wavefront_files.cpp
)

set(src_sandbox
    ${src_common}

    sandbox.cpp
)

set(src_test
    ${src_common}

    tests/test.cpp
    tests/t-index.cpp
)

# ---------------------------------------- End Declare Source Files ----------------------------------------------------

# ---------------------------------------- Begin Create Executable -----------------------------------------------------

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

macro(common_target_setup target)
    target_link_libraries(${target} PUBLIC core)
    target_link_libraries(${target} PRIVATE glfw OpenGL::GL)

    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_definitions(${target} PUBLIC
        "USE_ANSI_LOGGING=$<BOOL:${USE_ANSI_LOGGING}>"
        "IS_DEBUG=$<BOOL:${RENDERING_TECHNIQUES_DEBUG}>"
        "ASSETS_DIRECTORY=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\""
    )

    rendering_techniques_target_set_default_flags(${target} ${RENDERING_TECHNIQUES_DEBUG} false)
endmacro()

# if(HOT_SWAP)
#     # GUI shared library (reloadable)
#     add_library(${target_gui} SHARED ${src_gui})
#     target_link_libraries(${target_gui} PRIVATE core wayland-client)
#     common_target_setup(${target_gui})

#     # Force all symbols to be exported for debugging and hot-swap
#     target_compile_options(${target_gui} PRIVATE
#         -fvisibility=default
#     )
#     set_target_properties(${target_gui} PROPERTIES
#         CXX_VISIBILITY_PRESET default
#         VISIBILITY_INLINES_HIDDEN OFF
#     )

#     # Loader executable (for hot-swap in debug)
#     add_executable(${target_loader} main-loader.cpp)
#     target_link_libraries(${target_loader} PRIVATE dl)
#     common_target_setup(${target_loader})
#     target_compile_definitions(${target_loader} PUBLIC
#         GUI_LIB_FILE_NAME=\"lib${target_gui}.so\"
#         GUI_LIB_PATH=\"${PROJECT_BINARY_DIR}\"
#     )
#     target_include_directories(${target_loader} PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/lib/core/include
#     )
# else()
#     # GUI shared library (reloadable)
#     add_executable(${target_gui} main.cpp ${src_gui})
#     target_link_libraries(${target_gui} PUBLIC core wayland-client)
#     common_target_setup(${target_gui})
# endif()

add_executable(${target_sandbox} ${src_sandbox})
common_target_setup(${target_sandbox})

add_executable(${target_test} ${src_test})
common_target_setup(${target_test})

# ---------------------------------------- END Create Executable -------------------------------------------------------
