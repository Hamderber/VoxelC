#pragma once

#include <stdbool.h>
#include "rendering/camera/camera_t.h"

typedef union
{
    void *genericData;
    Camera_t *cameraData;
} EntityData_t;

typedef enum
{
    ENTITY_TYPE_GENERIC = 0,
    ENTITY_TYPE_CAMERA = 1,
} EntityType_t;

typedef struct
{
    bool heapAllocated;
    size_t refCount;
    EntityType_t type;
    EntityData_t data;
} Entity_t;