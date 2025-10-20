#include <stdlib.h>
#include <string.h>
#include "core/logs.h"
#include "core/types/scene_t.h"
#include "rendering/types/renderModel_t.h"

// Grow strategy: x2, starting from 4
static inline uint32_t scene_next_capacity(uint32_t current)
{
    return current ? (current << 1) : 4u;
}

void scene_modelAdd(Scene_t *scene, RenderModel_t mdl)
{
    if (!scene)
    {
        logs_log(LOG_ERROR, "scene_modelAdd: scene is NULL");
        return;
    }

    // Allocate or grow the pointer array if needed
    if (scene->modelCount >= scene->modelCapacity)
    {
        const uint32_t newCap = scene_next_capacity(scene->modelCapacity);
        void *newMem = realloc(scene->models, sizeof(RenderModel_t *) * newCap);
        if (!newMem)
        {
            logs_log(LOG_ERROR, "scene_modelAdd: realloc failed (from %u to %u)", scene->modelCapacity, newCap);
            return;
        }
        scene->models = (RenderModel_t **)newMem;
        scene->modelCapacity = newCap;
    }

    // Heap-allocate a copy of the model and store it
    RenderModel_t *stored = (RenderModel_t *)calloc(1, sizeof(RenderModel_t));
    if (!stored)
    {
        logs_log(LOG_ERROR, "scene_modelAdd: failed to allocate model slot");
        return;
    }
    // Shallow copy is intentional: we transfer ownership of handles/pointers
    // (buffers, views, sampler, descriptor sets) to the scene.
    *stored = mdl;

    scene->models[scene->modelCount++] = stored;

    logs_log(LOG_DEBUG, "scene_modelAdd: model added (count=%u, capacity=%u)",
             scene->modelCount, scene->modelCapacity);
}
