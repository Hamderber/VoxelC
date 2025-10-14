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

# Create executable
add_executable(VoxelC ${SOURCES})

# Add stb lib
target_include_directories(VoxelC PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)

# Include headers from src/ (so #include "Logger.h" works)
target_include_directories(VoxelC PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(VoxelC PRIVATE
    stbimg
    cgltf
    cJSON
)

# --- Find Vulkan ---
find_package(Vulkan REQUIRED)
target_link_libraries(VoxelC PRIVATE Vulkan::Vulkan)

# --- Add static GLFW manually ---
set(GLFW_INCLUDE_DIR "C:/vcpkg/installed/x64-windows-static/include")
set(GLFW_LIBRARY "C:/vcpkg/installed/x64-windows-static/lib/glfw3.lib")

target_include_directories(VoxelC PRIVATE ${GLFW_INCLUDE_DIR})
target_link_libraries(VoxelC PRIVATE ${GLFW_LIBRARY})

target_compile_definitions(VoxelC PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)