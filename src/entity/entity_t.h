#pragma once

#include <stdbool.h>
#include "rendering/camera/camera_t.h"

typedef enum
{
    ENTITY_COMPONENT_TYPE_GENERIC = 0,
    ENTITY_COMPONENT_TYPE_CAMERA = 1,
    ENTITY_COMPONENT_TYPE_PHYSICS = 2,
} EntityComponentType_t;

typedef struct
{
    // Use local axes for intention application (move/rotation)
    bool useLocalAxes;
    // Use this to determine velocity using an external normalized direction. (m/s)
    float uniformSpeed;
    // AI entities fill this from pathing (todo very future) or player-controlled inputs
    Vec3f_t moveIntention;
    // Use this for interpolation
    Vec3f_t posOld;
    Vec3f_t pos;
    Vec3f_t velocity;
    // This is where impulse is applied. Once acceleration impacts velocity in physics update it is set to 0 again
    Vec3f_t transientAcceleration;
    Vec3f_t gravity;
    // 0 = no drag 1 = stop immediately
    float drag;
    Quaternionf_t rotation;
    // Euler is way easier to use (obviously) and uses dt in physics
    Vec3f_t rotationIntentionEulerRad;
    // Does NOT use dt in physics. Use for instant rotation or time-domain inputs like player controlled input
    Quaternionf_t rotationIntentionQuat;
} EntityDataPhysics_t;

// Pointers only
typedef union
{
    void *genericData;
    EntityDataCamera_t *cameraData;
    EntityDataPhysics_t *physicsData;
} EntityComponentData_t;

typedef struct
{
    EntityComponentType_t type;
    EntityComponentData_t *data;
} EntityComponent_t;

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
    size_t componentCount;
    EntityComponent_t *components;
} Entity_t;