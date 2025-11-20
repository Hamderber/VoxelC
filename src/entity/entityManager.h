#pragma once

#include "core/types/state_t.h"
#include "entity/entityCollection_t.h"

enum EntityComponentType_e;
union EntityComponentData_t;

typedef struct EntityManger_t
{
    size_t collectionCount;
    struct EntityCollection_t entityCollections[ENTITY_COLLECTION_COUNT];
} EntityManger_t;

bool em_entityDataGet(struct Entity_t *e, enum EntityComponentType_e type, union EntityComponentData_t **outData);

bool em_entityIndexSingleton(struct EntityCollection_t *collection, struct Entity_t *e, size_t *index);

void em_entityAddToCollection(struct EntityCollection_t *collection, struct Entity_t *e);

void em_entityRemoveFromCollection(struct State_t *state, struct EntityCollection_t *collection, struct Entity_t *e);

struct Entity_t *em_entityCreateHeap(void);

struct Entity_t em_entityCreateStack(void);

void em_entityDestroy(struct State_t *state, struct Entity_t **e);

void em_create(struct State_t *state);

void em_destroy(struct State_t *state);

void em_init(struct State_t *state);