#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "core/crash_handler.h"
#include "renderGC.h"
#include "collection/dynamicStack_t.h"

static DynamicStack_t **ppFrameGarbage = NULL;

void renderGC_pushGarbage(const uint32_t FRAME_INDEX, VkBuffer buffer, VkDeviceMemory memory)
{
    PendingBufferDestroy_t *pGarbage = malloc(sizeof(PendingBufferDestroy_t));
    if (!pGarbage)
        return;

    pGarbage->buffer = buffer;
    pGarbage->memory = memory;

    dynamicStack_push(ppFrameGarbage[FRAME_INDEX], pGarbage);
}

void renderGC_flushGarbage(State_t *pState, const uint32_t FRAME_INDEX)
{
    DynamicStack_t *pDynamicStack = ppFrameGarbage[FRAME_INDEX];
    bool cont = true;
    while (cont)
    {
        PendingBufferDestroy_t *pPending = (PendingBufferDestroy_t *)dynamicStack_pop(pDynamicStack);
        cont = pPending;
        if (!cont)
            break;

        vkDestroyBuffer(pState->context.device, pPending->buffer, pState->context.pAllocator);
        vkFreeMemory(pState->context.device, pPending->memory, pState->context.pAllocator);
    }
}

void renderGC_init(State_t *pState)
{
    do
    {
        if (!pState)
            break;

        uint32_t maxFramesInFlight = pState->config.maxFramesInFlight;
        ppFrameGarbage = calloc(maxFramesInFlight, sizeof(DynamicStack_t *));
        if (!ppFrameGarbage)
            break;

        for (uint32_t i = 0; i < maxFramesInFlight; i++)
        {
            const size_t DEFAULT_CAPACITY = 1024;
            ppFrameGarbage[i] = dynamicStack_create(DEFAULT_CAPACITY);
        }

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without an intialized rendering garbage collector!");
}

void renderGC_destroy(State_t *pState)
{
    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        renderGC_flushGarbage(pState, i);
}