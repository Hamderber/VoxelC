#pragma region Includes
#pragma once
#include <stdlib.h>
#include "cmath/cmath.h"
#pragma endregion
#pragma region Defines
#define DYNAMIC_STACK_MAX_CAPACITY 16384
#define DYNAMIC_STACK_RESIZE_FACTOR 2
typedef struct DynamicStack_t
{
    size_t capacity;
    size_t index;
    void **ppCollection;
} DynamicStack_t;
#pragma endregion
#pragma region Operations
/// @brief Resizes the dynamic stack. Prevents resizing smaller than the size of the actual data stack.
/// Size [1, DYNAMIC_STACK_MAX_CAPACITY]
static inline bool dynamicStack_resize(DynamicStack_t *restrict pStack, size_t newSize)
{
    if (!pStack || !pStack->ppCollection || newSize == 0)
        return false;

    size_t clamped = cmath_clampSizet(newSize, 1, DYNAMIC_STACK_MAX_CAPACITY);

    // Resize would truncate the data
    if (clamped < pStack->index)
    {
        logs_log(LOG_WARN, "Attempted to shrink the size of a dynamic stack below the size of the tracked data!");
        return false;
    }

    if (clamped == pStack->capacity)
    {
        if (newSize > pStack->capacity)
            // Tried to grow but hit max cap
            return false;
        // No-op resize
        return true;
    }

    void **ppTmp = realloc(pStack->ppCollection, sizeof(void *) * newSize);
    if (!ppTmp)
        return false;

    pStack->ppCollection = ppTmp;
    pStack->capacity = newSize;

    // This should never happen, but its better to just avoid out of bounds stuff anyway
    if (pStack->index > pStack->capacity)
        pStack->index = pStack->capacity;

    return true;
}

/// @brief Adds pData to the top of the stack. Resizes the stack capacity by DYNAMIC_STACK_RESIZE_FACTOR if full, up to a max
/// size of DYNAMIC_STACK_MAX_CAPACITY.
static inline bool dynamicStack_push(DynamicStack_t *restrict pStack, void *restrict pData)
{
    if (!pStack || !pStack->ppCollection || !pData)
        return false;

    if (pStack->index == pStack->capacity)
    {
        size_t newSize = pStack->capacity * DYNAMIC_STACK_RESIZE_FACTOR;
        if (!dynamicStack_resize(pStack, newSize))
            return false;
    }

    pStack->ppCollection[pStack->index++] = pData;
    return true;
}

/// @brief Adds pData to the top of the stack only if the ADDRESS of pData is unique.
static inline bool dynamicStack_pushUnique(DynamicStack_t *restrict pStack, void *restrict pData)
{
    if (!pStack || !pStack->ppCollection || !pData)
        return false;

    for (size_t i = 0; i < pStack->index; i++)
        if (pStack->ppCollection[i] == pData)
            return false;

    return dynamicStack_push(pStack, pData);
}

/// @brief Removes the top data pointer from the stack, if one exists.
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
/// @brief Creates a stack with capacity [1, DYNAMIC_STACK_MAX_CAPACITY]
static inline DynamicStack_t *dynamicStack_create(size_t capacity)
{
    capacity = cmath_clampSizet(capacity, 1, DYNAMIC_STACK_MAX_CAPACITY);

    DynamicStack_t *pStack = calloc(1, sizeof(DynamicStack_t));
    if (!pStack)
        return NULL;

    pStack->ppCollection = calloc(capacity, sizeof(void *));
    if (!pStack->ppCollection)
    {
        free(pStack);
        return NULL;
    }

    pStack->index = 0;
    pStack->capacity = capacity;

    return pStack;
}

/// @brief Destroys the stack. DOES NOT free the data tracked in the stack. (Doesn't own those pointers)
static inline void dynamicStack_destroy(DynamicStack_t *pStack)
{
    if (!pStack)
        return;

    free(pStack->ppCollection);
    free(pStack);
}
#pragma endregion
#pragma region Undefines
#undef DYNAMIC_STACK_RESIZE_FACTOR
#pragma endregion