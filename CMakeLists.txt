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

add_library(stb STATIC lib/stb/stb_image_impl.c)
add_library(cgltf STATIC lib/stb/cgltf_impl.c)
target_include_directories(stb PUBLIC lib/stb)
target_compile_options(stb PRIVATE -w)

# Ignore stncpy warnings from cgltf
if(MSVC)
    target_compile_definitions(cgltf PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(cgltf PRIVATE /w)
else()
    target_compile_options(cgltf PRIVATE -w)
endif()

# Create executable
add_executable(VoxelC ${SOURCES})

# Add stb lib
target_include_directories(VoxelC PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib/stb
)

# Include headers from src/ (so #include "Logger.h" works)
target_include_directories(VoxelC PRIVATE ${CMAKE_SOURCE_DIR}/src)

# --- Find Vulkan ---
find_package(Vulkan REQUIRED)
target_link_libraries(VoxelC PRIVATE Vulkan::Vulkan)
target_link_libraries(VoxelC PRIVATE stb)
target_link_libraries(VoxelC PRIVATE stb cgltf)

# --- Add static GLFW manually ---
set(GLFW_INCLUDE_DIR "C:/vcpkg/installed/x64-windows-static/include")
set(GLFW_LIBRARY "C:/vcpkg/installed/x64-windows-static/lib/glfw3.lib")

target_include_directories(VoxelC PRIVATE ${GLFW_INCLUDE_DIR})
target_link_libraries(VoxelC PRIVATE ${GLFW_LIBRARY})

target_compile_definitions(VoxelC PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)