cmake_minimum_required(VERSION 3.10)
project(VoxelC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Collect all C source files in src/
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

# Output exe directly to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin")

# Add libs in lib/ folder
add_library(stbimg STATIC lib/stb_image_impl.c)
add_library(cgltf STATIC lib/cgltf_impl.c)
add_library(cJSON STATIC lib/cJSON.c)

target_include_directories(stbimg PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(cgltf PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_include_directories(cJSON PUBLIC ${CMAKE_SOURCE_DIR}/lib)

# Suppress warnings from included libs
if(MSVC)
    target_compile_definitions(cgltf PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(stbimg PRIVATE /w)
    target_compile_options(cgltf PRIVATE /w)
    target_compile_options(cJSON PRIVATE /w)
else()
    target_compile_options(stbimg PRIVATE -w)
    target_compile_options(cgltf PRIVATE -w)
    target_compile_options(cJSON PRIVATE -w)
endif()

add_library(VoxelC_Core INTERFACE)
target_include_directories(VoxelC_Core INTERFACE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)

# --- Find Vulkan ---
find_package(Vulkan REQUIRED)
target_link_libraries(VoxelC_Core INTERFACE Vulkan::Vulkan)

# --- Add static GLFW manually ---
set(GLFW_INCLUDE_DIR "C:/vcpkg/installed/x64-windows-static/include")
set(GLFW_LIBRARY "C:/vcpkg/installed/x64-windows-static/lib/glfw3.lib")

target_include_directories(VoxelC_Core INTERFACE ${GLFW_INCLUDE_DIR})
target_link_libraries(VoxelC_Core INTERFACE ${GLFW_LIBRARY})

add_executable(VoxelC ${SOURCES})

target_link_libraries(VoxelC PRIVATE
    VoxelC_Core
    stbimg
    cgltf
    cJSON
)

target_compile_definitions(VoxelC PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Super strict compiling for better code in the long run
if(MSVC)
    target_compile_options(VoxelC PRIVATE /W4 /WX)
else()
    target_compile_options(VoxelC PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)
enable_testing()

file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/*.c")
add_executable(VoxelC_Tests ${TEST_SOURCES})

target_link_libraries(VoxelC_Tests PRIVATE
    VoxelC_Core
    stbimg
    cgltf
    cJSON
)

add_dependencies(VoxelC_Tests VoxelC)
add_test(NAME VoxelC_UnitTests COMMAND VoxelC_Tests)

# On Windows we don’t link m.lib; only on Unix
if(NOT WIN32)
    target_link_libraries(VoxelC_Tests PRIVATE m)
endif()

# Run tests automatically after build only in Debug mode
if(CMAKE_CONFIGURATION_TYPES)
    add_custom_command(
        TARGET VoxelC_Tests
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests only in Debug mode..."
        COMMAND ${CMAKE_CTEST_COMMAND} -C Debug --output-on-failure --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests (Debug mode)..."
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
        TARGET VoxelC_Tests
        POST_BUILD
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests (Debug mode)..."
    )
endif()
