#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "core/crash_handler.h"
#include "renderGC.h"
#include "collection/dynamicStack_t.h"

static const size_t FLUSH_COUNT_PER_FRAME = 8;
static size_t *pCount = NULL;
static size_t *pCapacity = NULL;
static PendingBufferDestroy_t **ppPendingBufferDestroys = NULL;

void renderGC_pushGarbage(const uint32_t FRAME_INDEX, VkBuffer buffer, VkDeviceMemory memory)
{
    size_t count = pCount[FRAME_INDEX];
    size_t capacity = pCapacity[FRAME_INDEX];

    if (count >= capacity)
    {
        size_t newCap = capacity * 2;
        PendingBufferDestroy_t *pNew = realloc(ppPendingBufferDestroys[FRAME_INDEX], newCap * sizeof(PendingBufferDestroy_t));

        if (!pNew)
            crashHandler_crash_graceful(CRASH_LOCATION, "Failed to grow rendering garbage collector array!");

        ppPendingBufferDestroys[FRAME_INDEX] = pNew;
        pCapacity[FRAME_INDEX] = newCap;
        capacity = newCap;
    }

    ppPendingBufferDestroys[FRAME_INDEX][count] = (PendingBufferDestroy_t){
        .buffer = buffer,
        .memory = memory};

    pCount[FRAME_INDEX] = count + 1;
}

void renderGC_flushGarbage(State_t *pState, const uint32_t FRAME_INDEX, const bool FLUSH_ALL)
{
    if (!pState)
        return;

    size_t count = pCount[FRAME_INDEX];
    if (count == 0)
        return;

    PendingBufferDestroy_t *pGarbage = ppPendingBufferDestroys[FRAME_INDEX];

    size_t flushCount = 0;
    for (size_t i = 0; i < count; ++i)
    {
        VkBuffer buffer = pGarbage[i].buffer;
        VkDeviceMemory memory = pGarbage[i].memory;

        if (buffer != VK_NULL_HANDLE)
            vkDestroyBuffer(pState->context.device, buffer, pState->context.pAllocator);
        if (memory != VK_NULL_HANDLE)
            vkFreeMemory(pState->context.device, memory, pState->context.pAllocator);

        pCount[FRAME_INDEX]--;

        flushCount++;
        if (flushCount >= FLUSH_COUNT_PER_FRAME && !FLUSH_ALL)
            break;
    }
}

void renderGC_init(State_t *pState)
{
    bool crash = false;
    do
    {
        if (!pState)
            break;

        uint32_t maxFramesInFlight = pState->config.maxFramesInFlight;
        ppPendingBufferDestroys = calloc(maxFramesInFlight, sizeof(PendingBufferDestroy_t *));
        pCount = calloc(maxFramesInFlight, sizeof(size_t));
        pCapacity = calloc(maxFramesInFlight, sizeof(size_t));
        if (!ppPendingBufferDestroys || !pCount || !pCapacity)
            break;

        for (uint32_t i = 0; i < maxFramesInFlight; i++)
        {
            const size_t DEFAULT_CAPACITY = 256;
            ppPendingBufferDestroys[i] = malloc(sizeof(PendingBufferDestroy_t) * DEFAULT_CAPACITY);
            pCapacity[i] = DEFAULT_CAPACITY;
            pCount[i] = 0;
            if (!ppPendingBufferDestroys[i])
            {
                crash = true;
                break;
            }
        }

        if (!crash)
            return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without an intialized rendering garbage collector!");
}

void renderGC_destroy(State_t *pState)
{
    if (!pState || !ppPendingBufferDestroys)
        return;

    uint32_t maxFramesInFlight = pState->config.maxFramesInFlight;

    // Flush dangling garbage
    for (uint32_t i = 0; i < maxFramesInFlight; ++i)
    {
        renderGC_flushGarbage(pState, i, true);
        free(ppPendingBufferDestroys[i]);
    }

    free(ppPendingBufferDestroys);
    free(pCount);
    free(pCapacity);

    ppPendingBufferDestroys = NULL;
    pCount = NULL;
    pCapacity = NULL;
}