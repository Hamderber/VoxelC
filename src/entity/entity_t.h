#pragma once

#include <stdbool.h>
#include "core/logs.h"
#include "world/chunk.h"
#include "rendering/camera/camera_t.h"
#include "collection/flags64_t.h"
#include "character/character.h"

typedef enum EntityComponentType_t
{
    ENTITY_COMPONENT_TYPE_GENERIC = 0,
    ENTITY_COMPONENT_TYPE_CAMERA = 1,
    ENTITY_COMPONENT_TYPE_PHYSICS = 2,
    ENTITY_COMPONENT_TYPE_CHUNK = 3,
} EntityComponentType_t;

typedef struct EntityDataPhysics_t
{
    // Use local axes for intention application (move/rotation)
    bool useLocalAxes;
    // Use this to determine velocity using an external normalized direction. (m/s)
    float uniformSpeed;
    // Default speed. Useful for setting speed directly to this after a speed modification instead of doing things like
    // multiplying and dividing by a multiplier, which introduces precision errors
    float uniformSpeedBase;
    // AI entities fill this from pathing (todo very future) or player-controlled inputs
    Vec3f_t moveIntention;
    // Use this for interpolation
    Vec3f_t worldPosOld;
    Vec3f_t worldPos;
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
    // Current entity's chunk position (NOT world pos)
    Vec3i_t chunkPos;
} EntityDataPhysics_t;

typedef struct EntityDataChunk_t
{
    // This is NOT for tracking an entity's chunk position in regards to physics
    Vec3i_t chunkPos;
    // Should the chunkPos be updated this frame?
    bool dirty;
} EntityDataChunk_t;

// Pointers only
typedef union EntityComponentData_t
{
    void *pGenericData;
    Camera_t *pCameraData;
    struct EntityDataPhysics_t *pPhysicsData;
    struct EntityDataChunk_t *pChunkData;
} EntityComponentData_t;

typedef struct EntityComponent_t
{
    enum EntityComponentType_t type;
    union EntityComponentData_t *pComponentData;
} EntityComponent_t;

typedef enum EntityType_t
{
    ENTITY_TYPE_GENERIC = 0,
    ENTITY_TYPE_CAMERA = 1,
    ENTITY_TYPE_CREATURE = 2,
    // An item floating in the world (not in an inventory)
    ENTITY_TYPE_ITEM_WORLD = 3,
    ENTITY_TYPE_COUNT,
} EntityType_t;

typedef struct Entity_t
{
    bool heapAllocated;
    size_t refCount;
    enum EntityType_t type;
    size_t componentCount;
    struct EntityComponent_t *pComponents;
    flags64_t entityFlags;
} Entity_t;

#pragma region Entity Flags
/// @brief Flags for an entity. Current range is [0, 63]
typedef enum EntityFlags_t
{
    ENTITY_FLAG_PLAYER = 0,
    ENTITY_FLAG_COUNT,
} EntityFlags_t;
static inline bool entity_flag_isPlayer(Entity_t *pEntity)
{
    if (!pEntity)
        return false;

    return flag64_get(pEntity->entityFlags, ENTITY_FLAG_PLAYER);
}
#pragma endregion
#pragma region Entity
struct State_t;
/// @brief Updates the entity's chunk position
static void entity_chunkPos_update(struct State_t *pState, Entity_t *pEntity, EntityComponentData_t *pComponentData)
{
#ifdef DEBUG
    const Vec3i_t OLD = worldPosf_to_chunkPos(pComponentData->pPhysicsData->worldPosOld);
    const Vec3i_t NEW = worldPosf_to_chunkPos(pComponentData->pPhysicsData->worldPos);

    bool chunkChanged = !cmath_vec3i_equals(OLD, NEW, 0);
    if (chunkChanged)
    {
        pComponentData->pPhysicsData->chunkPos = NEW;

        /*
            TODO: Add flag to chunks to track what (player) entity is loading it
                  Add chunks to chunkmanager for rendering
                  store cpu side chunk and render chunk separately and delete renter when out of render distance and
                  unload cpu when not in simulation distance (config needs added and a region of world to the config)
        */
        logs_log(LOG_DEBUG, "Entity %p ChunkPos (%d, %d, %d) -> (%d, %d, %d).", pEntity, OLD.x, OLD.y, OLD.z, NEW.x, NEW.y, NEW.z);
    }

    if (chunkChanged && entity_flag_isPlayer(pEntity))
    {
        logs_log(LOG_DEBUG, "Entity %p is a player. Updating world and render chunks.", pEntity);
        entity_player_chunkPos_update_publish(pState, pEntity, pComponentData);
    }
#else
    pState;
    pEntity;
    // Just set the chunkpos directly
    pComponentData->pPhysicsData->chunkPos = worldPosf_to_chunkPos(pComponentData->pPhysicsData->worldPos);
#endif
}
#pragma endregion