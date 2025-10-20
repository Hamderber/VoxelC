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

void scene_modelAdd(Scene_t *scene, RenderModel_t *mdl)
{
    if (!scene)
        return;

    if (scene->modelCount >= scene->modelCapacity)
    {
        uint32_t newCap = scene_next_capacity(scene->modelCapacity);
        void *newMem = realloc(scene->models, sizeof(RenderModel_t *) * newCap);
        if (!newMem)
            return;
        scene->models = newMem;
        scene->modelCapacity = newCap;
    }

    scene->models[scene->modelCount++] = mdl; // just store pointer
    logs_log(LOG_DEBUG, "scene_modelAdd: model added (count=%u, cap=%u)",
             scene->modelCount, scene->modelCapacity);
}
