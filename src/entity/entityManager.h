#pragma once

#include "core/types/state_t.h"
#include "entity/entityCollection_t.h"
#include "entity/entity_t.h"

typedef struct
{
    size_t collectionCount;
    EntityCollection_t entityCollections[ENTITY_COLLECTION_COUNT];
} EntityManger_t;

bool em_entityDataGet(Entity_t *e, EntityComponentType_t type, EntityComponentData_t **outData);

bool em_entityIndexSingleton(EntityCollection_t *collection, Entity_t *e, size_t *index);

void em_entityAddToCollection(EntityCollection_t *collection, Entity_t *e);

void em_entityRemoveFromCollection(struct State_t *state, EntityCollection_t *collection, Entity_t *e);

Entity_t *em_entityCreateHeap(void);

Entity_t em_entityCreateStack(void);

void em_entityDestroy(struct State_t *state, Entity_t **e);

void em_create(struct State_t *state);

void em_destroy(struct State_t *state);

void em_init(struct State_t *state);