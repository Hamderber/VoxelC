#include "core/types/state_t.h"
#include "entity/entity_t.h"
#include "entity/entityCollection_t.h"
#include "core/logs.h"
#include <stdlib.h>
#include <string.h>

bool em_entityDataGet(Entity_t *e, EntityComponentType_t type, EntityComponentData_t **outData)
{
    if (!e || !outData || e->componentCount == 0)
        return false;

    for (size_t i = 0; i < e->componentCount; i++)
    {
        if (e->pComponents[i].type == type)
        {
            *outData = e->pComponents[i].pComponentData;
            // logs_log(LOG_DEBUG, "Found pComponentData type %d for entity %p at component address %p",
            //          (int)type, (void *)e, (void *)*outData);
            return true;
        }
    }

    logs_log(LOG_WARN, "Failed to find datatype %d for entity %p!", (int)type, (void *)e);
    return false;
}

bool em_entityIndexSingleton(EntityCollection_t *collection, Entity_t *e, size_t *index)
{
    // Implementation is similar to eventBus listerner singleton add
    bool indexFound = false;

    for (size_t i = 0; i < ENTITIES_MAX_IN_COLLECTION; i++)
    {
        if (collection->entities[i] == e)
        {
            logs_log(LOG_ERROR, "Attempted to add to entity colletion %p with duplicate entity %p!",
                     (void *)collection, (void *)e);

            return false;
        }

        // Assign the value of the first available index found
        if (!indexFound && collection->entities[i] == NULL)
        {
            *index = i;
            indexFound = true;
        }
    }

    if (indexFound)
    {
        return true;
    }
    else
    {
        logs_log(LOG_ERROR, "Failed to assign an index for entity %p. There are ENTITIES_MAX_IN_COLLECTION [%d] assigned!",
                 (void *)e, (int)ENTITIES_MAX_IN_COLLECTION);
        return false;
    }
}

void em_entityAddToCollection(EntityCollection_t *collection, Entity_t *e)
{
    if (!(collection == NULL || e == NULL || collection->entities == NULL))
    {
        size_t index;
        if (em_entityIndexSingleton(collection, e, &index))
        {
            e->refCount++;
            collection->entities[index] = e;
            logs_log(LOG_DEBUG, "Added entity %p to collection %p at index %d. It now has %d reference(s).",
                     (void *)e, (void *)collection, (int)index, (int)e->refCount);
            return;
        }
    }

    logs_log(LOG_ERROR, "Failed to add entity (%p) to collection (%p)'s entity array (%p [%d])!",
             (void *)e, (void *)collection, (void *)collection->entities, (int)ENTITIES_MAX_IN_COLLECTION);
}

void em_entityRemoveFromCollection(State_t *state, EntityCollection_t *collection, Entity_t *e)
{
    if (!(collection == NULL || e == NULL || collection->entities == NULL))
    {
        for (size_t i = 0; i < ENTITIES_MAX_IN_COLLECTION; i++)
        {
            if (collection->entities[i] == e)
            {
                e->refCount--;
                collection->entities[i] = NULL;
                logs_log(LOG_DEBUG, "Removed entity %p from collection %p. It now has %d reference(s).",
                         (void *)e, (void *)collection, (int)e->refCount);

                // Automatically destroy danlging entities
                if (e->refCount == 0)
                {
                    em_entityDestroy(state, &e);
                }

                return;
            }
        }
    }

    logs_log(LOG_ERROR, "Failed to remove entity (%p) from collection (%p)'s entity array (%p [%d])!",
             (void *)e, (void *)collection, (void *)collection->entities);
}

void em_entityPurgeFromCollections(EntityManger_t *entityManager, Entity_t *e)
{
    if (entityManager == NULL || e == NULL)
    {
        logs_log(LOG_ERROR, "Failed to purge entity %p from collections in entity manager %p!",
                 (void *)entityManager, (void *)e);
        return;
    }

    for (size_t i = 0; i < ENTITY_COLLECTION_COUNT; i++)
    {
        if (entityManager->entityCollections[i].entities == NULL)
        {
            // This isn't necessarily an error because the collections could be actively being nulled during entity
            // manager destruction
            continue;
        }
        for (size_t j = 0; j < ENTITIES_MAX_IN_COLLECTION; j++)
        {
            if (entityManager->entityCollections[i].entities[j] == e)
            {
                entityManager->entityCollections[i].entities[j] = NULL;
                e->refCount--;

                logs_log(LOG_DEBUG, "Removed entity %p from collection %p. It now has %d reference(s).",
                         (void *)e, (void *)entityManager->entityCollections[i].entities, (int)e->refCount);
            }
        }
    }

    if (e->refCount != 0)
    {
        logs_log(LOG_ERROR, "Failed to fully purge entity %p from the collections in entity manager %p!",
                 (void *)e, (void *)entityManager);
    }
}

/// @brief Creates an entity struct configured for storage on the heap
/// @param  void
/// @return Entity_t
Entity_t *em_entityCreateHeap(void)
{
    Entity_t *e = calloc(1, sizeof(Entity_t));
    e->heapAllocated = true;
    return e;
}

/// @brief Creates an entity struct configured for storage on the stack
/// @param  void
/// @return Entity_t
Entity_t em_entityCreateStack(void)
{
    Entity_t e = {0};
    e.heapAllocated = false;
    return e;
}

/// @brief Destroys the entity and conditionally frees depending on storage location
/// @param e
void em_entityDestroy(State_t *state, Entity_t **e)
{
    if (!e || !*e)
        return;

    Entity_t *entity = *e;
    logs_log(LOG_DEBUG, "Destroying entity %p", (void *)entity);

    em_entityPurgeFromCollections(&state->entityManager, entity);

    // Free internal components if present
    for (size_t i = 0; i < entity->componentCount; i++)
    {
        if (entity->pComponents[i].pComponentData != NULL)
        {
            free((void *)entity->pComponents[i].pComponentData->pGenericData);
            free(entity->pComponents[i].pComponentData);
            entity->pComponents[i].pComponentData = NULL;
        }
    }

    free(entity->pComponents);
    entity->pComponents = NULL;

    // Only free the struct itself if it was heap-allocated
    if (entity->heapAllocated)
    {
        free(entity);
    }

    *e = NULL;
}

void em_create(State_t *state)
{
    state->entityManager.collectionCount = ENTITY_COLLECTION_COUNT;
    for (size_t i = 0; i < ENTITY_COLLECTION_COUNT; i++)
    {
        state->entityManager.entityCollections[i].entities = calloc(ENTITIES_MAX_IN_COLLECTION, sizeof(Entity_t *));
    }
}

void em_destroy(State_t *state)
{
    for (size_t i = 0; i < ENTITY_COLLECTION_COUNT; i++)
    {
        for (size_t j = 0; j < ENTITIES_MAX_IN_COLLECTION; j++)
        {
            em_entityDestroy(state, &state->entityManager.entityCollections[i].entities[j]);
        }

        free(state->entityManager.entityCollections[i].entities);
        state->entityManager.entityCollections[i].entities = NULL;
    }
}

void em_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing entity manager...");
    em_create(state);
}