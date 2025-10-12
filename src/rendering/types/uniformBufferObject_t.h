#include "c_math/c_math.h"
#include <stdalign.h>

// https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/
// Struct members are the same as the shader codes'
typedef struct
{
    // It is critical to make sure that the sizing is
    // alligned for use with vulkan and shaders. 16 per row.
    // So for values less than 16 do Ex: alignas(16) float foo;
    // Just have everything start with alignas for safety
    alignas(16) Mat4c_t model;
    alignas(16) Mat4c_t view;
    alignas(16) Mat4c_t projection;
} UniformBufferObject_t;
