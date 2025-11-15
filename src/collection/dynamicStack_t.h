#pragma region Includes
#pragma once
#include <stdlib.h>
#include "cmath/cmath.h"
#pragma endregion
#pragma region Defines
#define DYNAMIC_STACK_MAX_CAPACITY 16384
typedef struct DynamicStack_t
{
    size_t defaultCapacity;
    size_t capacity;
    size_t index;
    void **ppCollection;
} DynamicStack_t;
#pragma endregion
#pragma region Operations
static inline bool dynamicStack_push(DynamicStack_t *restrict pStack, void *restrict pData)
{
    if (!pStack || !pStack->ppCollection || !pData)
        return false;

    if (pStack->index == pStack->capacity)
    {
        size_t newSize = pStack->capacity * 2;
        void **ppTmp = realloc(pStack->ppCollection, sizeof(void *) * newSize);
        if (!ppTmp)
            return false;

        pStack->ppCollection = ppTmp;
        pStack->capacity = newSize;
    }

    pStack->ppCollection[pStack->index++] = pData;
    return true;
}

static inline bool dynamicStack_pushUnique(DynamicStack_t *restrict pStack, void *restrict pData)
{
    if (!pStack || !pStack->ppCollection || !pData)
        return false;

    for (size_t i = 0; i < pStack->index; i++)
        if (pStack->ppCollection[i] == pData)
            return false;

    return dynamicStack_push(pStack, pData);
}

static inline void *dynamicStack_pop(DynamicStack_t *pStack)
{
    if (!pStack || !pStack->ppCollection)
        return NULL;

    // Nothing to pop
    if (pStack->index == 0)
        return NULL;

    void *pData = pStack->ppCollection[--pStack->index];
    return pData;
}
#pragma endregion
#pragma region Create/Destroy
static inline DynamicStack_t *dynamicStack_create(size_t defaultCapacity)
{
    defaultCapacity = cmath_clampSizet(defaultCapacity, 1, DYNAMIC_STACK_MAX_CAPACITY);

    DynamicStack_t *pStack = calloc(1, sizeof(DynamicStack_t));
    if (!pStack)
        return NULL;

    pStack->ppCollection = calloc(defaultCapacity, sizeof(void *));
    if (!pStack->ppCollection)
    {
        free(pStack);
        return NULL;
    }

    pStack->index = 0;
    pStack->capacity = defaultCapacity;
    pStack->defaultCapacity = defaultCapacity;

    return pStack;
}

static inline void dynamicStack_destroy(DynamicStack_t *pStack)
{
    if (!pStack)
        return;

    free(pStack->ppCollection);
    free(pStack);
}
#pragma endregion