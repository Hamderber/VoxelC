#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "core/crash_handler.h"
#include "renderGC.h"
#include "collection/dynamicStack_t.h"

static VkDevice vkDevice = {0};
static uint32_t maxFramesInFlight = 0;
static VkAllocationCallbacks *pAllocator = NULL;
static const size_t FLUSH_COUNT_PER_FRAME = 8;
static size_t *pCount = NULL;
static size_t *pCapacity = NULL;
static PendingBufferDestroy_t **ppPendingBufferDestroys = NULL;
static uint32_t frameIndex = 0;

void renderGC_updateFrame(const uint32_t FRAME_INDEX) { frameIndex = FRAME_INDEX; }

void renderGC_pushGarbage(VkBuffer buffer, VkDeviceMemory memory)
{
    size_t count = pCount[frameIndex];
    size_t capacity = pCapacity[frameIndex];

    if (count >= capacity)
    {
        size_t newCap = capacity * 2;
        PendingBufferDestroy_t *pNew = realloc(ppPendingBufferDestroys[frameIndex], newCap * sizeof(PendingBufferDestroy_t));

        if (!pNew)
            crashHandler_crash_graceful(CRASH_LOCATION, "Failed to grow rendering garbage collector array!");

        ppPendingBufferDestroys[frameIndex] = pNew;
        pCapacity[frameIndex] = newCap;
        capacity = newCap;
    }

    ppPendingBufferDestroys[frameIndex][count] = (PendingBufferDestroy_t){
        .buffer = buffer,
        .memory = memory};

    pCount[frameIndex] = count + 1;
}

void renderGC_flushGarbage(const uint32_t FRAME_INDEX, const bool FLUSH_ALL)
{
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
            vkDestroyBuffer(vkDevice, buffer, pAllocator);
        if (memory != VK_NULL_HANDLE)
            vkFreeMemory(vkDevice, memory, pAllocator);

        pCount[FRAME_INDEX]--;

        flushCount++;
        if (flushCount >= FLUSH_COUNT_PER_FRAME && !FLUSH_ALL)
            break;
    }
}

void renderGC_init(VkDevice device, uint32_t maxFramesInFlightCfg, VkAllocationCallbacks *pAllocatorCtx)
{
    bool crash = false;
    do
    {
        vkDevice = device;
        maxFramesInFlight = maxFramesInFlightCfg;
        pAllocator = pAllocatorCtx;

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

void renderGC_destroy(void)
{
    if (!ppPendingBufferDestroys)
        return;

    // Flush dangling garbage
    for (uint32_t i = 0; i < maxFramesInFlight; ++i)
    {
        const bool FLUSH_ALL = true;
        renderGC_flushGarbage(i, FLUSH_ALL);
        free(ppPendingBufferDestroys[i]);
    }

    free(ppPendingBufferDestroys);
    free(pCount);
    free(pCapacity);

    ppPendingBufferDestroys = NULL;
    pCount = NULL;
    pCapacity = NULL;
}