cmake_minimum_required(VERSION 3.10)
project(VoxelC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ======================================================================
# Collect sources
# ======================================================================
file(GLOB_RECURSE ALL_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

# Set your entry point (main) file explicitly:
set(APP_MAIN "${CMAKE_SOURCE_DIR}/src/app.c")

# Engine sources are everything except the entry point
set(ENGINE_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM ENGINE_SOURCES "${APP_MAIN}")

# Tests
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/*.c")

# ======================================================================
# Output configuration
# ======================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin")

# ======================================================================
# Third-party static libs
# ======================================================================
add_library(stbimg STATIC lib/stb_image_impl.c)
add_library(cgltf  STATIC lib/cgltf_impl.c)
add_library(cJSON  STATIC lib/cJSON.c)

target_include_directories(stbimg PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(cgltf  PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(cJSON  PUBLIC ${CMAKE_SOURCE_DIR}/lib)

# Silence 3rd-party warnings
if(MSVC)
    target_compile_definitions(cgltf PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(stbimg PRIVATE /w)
    target_compile_options(cgltf  PRIVATE /w)
    target_compile_options(cJSON  PRIVATE /w)
else()
    target_compile_options(stbimg PRIVATE -w)
    target_compile_options(cgltf  PRIVATE -w)
    target_compile_options(cJSON  PRIVATE -w)
endif()

# ======================================================================
# Public headers (include paths) + system libs
# ======================================================================
add_library(VoxelC_Core INTERFACE)
target_include_directories(VoxelC_Core INTERFACE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(VoxelC_Core INTERFACE Vulkan::Vulkan)

# GLFW (vcpkg static)
set(GLFW_INCLUDE_DIR "C:/vcpkg/installed/x64-windows-static/include")
set(GLFW_LIBRARY     "C:/vcpkg/installed/x64-windows-static/lib/glfw3.lib")
target_include_directories(VoxelC_Core INTERFACE ${GLFW_INCLUDE_DIR})
target_link_libraries(VoxelC_Core INTERFACE ${GLFW_LIBRARY})

# ======================================================================
# Engine library (no main)
# ======================================================================
add_library(VoxelC_Lib STATIC ${ENGINE_SOURCES})
target_link_libraries(VoxelC_Lib PUBLIC
    VoxelC_Core
    stbimg
    cgltf
    cJSON
)

# Strict warnings
if(MSVC)
    target_compile_options(VoxelC_Lib PRIVATE /W4 /WX)
else()
    target_compile_options(VoxelC_Lib PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_compile_definitions(VoxelC_Lib PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# ======================================================================
# Application executable (needs at least one source)
# ======================================================================
add_executable(VoxelC ${APP_MAIN})
target_link_libraries(VoxelC PRIVATE VoxelC_Lib)

# Match warnings on the exe too (optional but nice)
if(MSVC)
    target_compile_options(VoxelC PRIVATE /W4)
else()
    target_compile_options(VoxelC PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ======================================================================
# Tests
# ======================================================================
include(CTest)
enable_testing()

add_executable(VoxelC_Tests ${TEST_SOURCES})
target_link_libraries(VoxelC_Tests PRIVATE VoxelC_Lib)

# Link libm only on non-Windows if needed by tests
if(NOT WIN32)
    target_link_libraries(VoxelC_Tests PRIVATE m)
endif()

add_test(NAME VoxelC_UnitTests COMMAND VoxelC_Tests)

# Auto-run tests post-build (Debug only) — uses CTest (no PowerShell)
if(CMAKE_CONFIGURATION_TYPES)
    add_custom_command(
        TARGET VoxelC_Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests only in Debug mode..."
        COMMAND ${CMAKE_CTEST_COMMAND} -C Debug --output-on-failure --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests (Debug mode)..."
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET VoxelC_Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests (Debug)..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests (Debug mode)..."
    )
endif()

# ======================================================================
# Prevent MSYS/MinGW from forcing sh; avoids shell weirdness on Win
# ======================================================================
if(WIN32)
    set(CMAKE_SH "CMAKE_SH-NOTFOUND")
endif()
