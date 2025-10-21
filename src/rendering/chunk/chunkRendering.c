#include <vulkan/vulkan.h>
#include "cmath/cmath.h"
#include <stdlib.h>
#include "core/types/state_t.h"
#include "rendering/types/renderChunk_t.h"

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position)
{
    chunk->modelMatrix = cmath_mat_rotate(MAT4_IDENTITY, 0.0F, VEC3_Y_AXIS);
    chunk->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, *position);
}

void chunk_renderDestroy(State_t *state, RenderChunk_t *chunk)
{
    if (!chunk)
        return;

    const VkDevice device = state->context.device;
    const VkAllocationCallbacks *alloc = state->context.pAllocator;

    // Destroy buffers first, then free memory they were bound to
    if (chunk->vertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, chunk->vertexBuffer, alloc);
        chunk->vertexBuffer = VK_NULL_HANDLE;
    }
    if (chunk->vertexMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, chunk->vertexMemory, alloc);
        chunk->vertexMemory = VK_NULL_HANDLE;
    }

    if (chunk->indexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, chunk->indexBuffer, alloc);
        chunk->indexBuffer = VK_NULL_HANDLE;
    }
    if (chunk->indexMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, chunk->indexMemory, alloc);
        chunk->indexMemory = VK_NULL_HANDLE;
    }

    free(chunk);
}