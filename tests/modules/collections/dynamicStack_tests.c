#include "../../unit_tests.h"
#include <stdbool.h>
#include "collection/dynamicStack_t.h"

static int fails = 0;

static bool test_dynamicStack_create_and_clamp(void)
{
    // Capacity 0 should clamp to [1, DYNAMIC_STACK_MAX_CAPACITY]
    DynamicStack_t *pStack0 = dynamicStack_create(0);
    if (!pStack0)
        return false;

    if (!pStack0->ppCollection)
        return false;
    if (pStack0->capacity < 1 || pStack0->capacity > DYNAMIC_STACK_MAX_CAPACITY)
        return false;
    if (pStack0->index != 0)
        return false;

    dynamicStack_destroy(pStack0);

    // Oversized requested capacity must clamp to DYNAMIC_STACK_MAX_CAPACITY
    DynamicStack_t *pStackMax = dynamicStack_create(DYNAMIC_STACK_MAX_CAPACITY + 1000);
    if (!pStackMax)
        return false;

    if (!pStackMax->ppCollection)
        return false;
    if (pStackMax->capacity != DYNAMIC_STACK_MAX_CAPACITY)
        return false;
    if (pStackMax->index != 0)
        return false;

    dynamicStack_destroy(pStackMax);
    return true;
}

static bool test_dynamicStack_push_pop_order(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    int a = 1, b = 2, c = 3;

    if (!dynamicStack_push(pStack, &a))
        return false;
    if (!dynamicStack_push(pStack, &b))
        return false;
    if (!dynamicStack_push(pStack, &c))
        return false;

    if (pStack->index != 3)
        return false;

    // LIFO order: c, b, a
    void *p = NULL;

    p = dynamicStack_pop(pStack);
    if (p != &c || pStack->index != 2)
        return false;

    p = dynamicStack_pop(pStack);
    if (p != &b || pStack->index != 1)
        return false;

    p = dynamicStack_pop(pStack);
    if (p != &a || pStack->index != 0)
        return false;

    // Pop from empty stack -> NULL, index stays 0
    p = dynamicStack_pop(pStack);
    if (p != NULL || pStack->index != 0)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_resize_grow_and_shrink(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    int a = 1, b = 2, c = 3;

    // Push three items
    if (!dynamicStack_push(pStack, &a) ||
        !dynamicStack_push(pStack, &b) ||
        !dynamicStack_push(pStack, &c))
        return false;

    if (pStack->index != 3 || pStack->capacity != 4)
        return false;

    // Grow to larger capacity within max
    size_t oldCap = pStack->capacity;
    if (!dynamicStack_resize(pStack, 10))
        return false;

    if (pStack->capacity < 10) // clamped up to max, but must be >= requested
        return false;
    if (pStack->index != 3)
        return false;
    if (pStack->ppCollection[0] != &a ||
        pStack->ppCollection[1] != &b ||
        pStack->ppCollection[2] != &c)
        return false;
    if (pStack->capacity <= oldCap)
        return false;

    // Shrink down but not below index (3)
    oldCap = pStack->capacity;
    if (!dynamicStack_resize(pStack, 3))
        return false;

    if (pStack->capacity != 3)
        return false;
    if (pStack->index != 3)
        return false;
    if (pStack->ppCollection[0] != &a ||
        pStack->ppCollection[1] != &b ||
        pStack->ppCollection[2] != &c)
        return false;

    // Attempt to shrink below index should fail and keep capacity unchanged
    oldCap = pStack->capacity;
    if (dynamicStack_resize(pStack, 2) != false)
        return false;
    if (pStack->capacity != oldCap || pStack->index != 3)
        return false;

    // Resize to same capacity is a no-op success
    if (!dynamicStack_resize(pStack, pStack->capacity))
        return false;
    if (pStack->capacity != oldCap)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_push_resize_preserves_data(void)
{
    // Start with very small capacity to force a resize
    DynamicStack_t *pStack = dynamicStack_create(2);
    if (!pStack)
        return false;

    size_t initialCapacity = pStack->capacity;
    if (initialCapacity < 2)
        return false;

    int a = 1, b = 2, c = 3;

    if (!dynamicStack_push(pStack, &a))
        return false;
    if (!dynamicStack_push(pStack, &b))
        return false;

    // Next push should trigger a resize via dynamicStack_resize
    if (!dynamicStack_push(pStack, &c))
        return false;

    if (pStack->index != 3)
        return false;
    if (pStack->capacity <= initialCapacity)
        return false; // must have grown

    // Data must be preserved in correct order in underlying array
    if (pStack->ppCollection[0] != &a ||
        pStack->ppCollection[1] != &b ||
        pStack->ppCollection[2] != &c)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_pushUnique_behavior(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    int a = 1, b = 2;

    // First unique push of &a should succeed
    if (!dynamicStack_pushUnique(pStack, &a))
        return false;
    if (pStack->index != 1)
        return false;

    // Duplicate &a should be rejected, no growth
    if (dynamicStack_pushUnique(pStack, &a) != false)
        return false;
    if (pStack->index != 1)
        return false;

    // New pointer &b should be allowed
    if (!dynamicStack_pushUnique(pStack, &b))
        return false;
    if (pStack->index != 2)
        return false;

    if (pStack->ppCollection[0] != &a ||
        pStack->ppCollection[1] != &b)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_pushUnique_pop_readd(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    int a = 1;

    if (!dynamicStack_pushUnique(pStack, &a))
        return false;
    if (pStack->index != 1)
        return false;

    // Pop it off
    void *p = dynamicStack_pop(pStack);
    if (p != &a || pStack->index != 0)
        return false;

    // Now pushUnique(&a) must succeed again
    if (!dynamicStack_pushUnique(pStack, &a))
        return false;
    if (pStack->index != 1)
        return false;
    if (pStack->ppCollection[0] != &a)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_shrink_then_grow_via_push(void)
{
    // Use bigger initial capacity to have room
    DynamicStack_t *pStack = dynamicStack_create(8);
    if (!pStack)
        return false;

    int vals[10];
    for (int i = 0; i < 10; ++i)
        vals[i] = i + 1;

    // Push 3 elements
    if (!dynamicStack_push(pStack, &vals[0]) ||
        !dynamicStack_push(pStack, &vals[1]) ||
        !dynamicStack_push(pStack, &vals[2]))
        return false;

    if (pStack->index != 3)
        return false;

    // Shrink capacity to exactly index (3)
    if (!dynamicStack_resize(pStack, 3))
        return false;
    if (pStack->capacity != 3 || pStack->index != 3)
        return false;

    // Now push more elements; this will eventually force a grow
    size_t capBefore = pStack->capacity;
    for (int i = 3; i < 10; ++i)
    {
        if (!dynamicStack_push(pStack, &vals[i]))
            return false;
    }

    if (pStack->index != 10)
        return false;
    if (pStack->capacity <= capBefore)
        return false; // must have grown

    // Pop all and ensure LIFO and pointer integrity
    for (int i = 9; i >= 0; --i)
    {
        void *p = dynamicStack_pop(pStack);
        if (p != &vals[i])
            return false;
    }
    if (pStack->index != 0)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_resize_with_null_collection(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    size_t oldCap = pStack->capacity;
    size_t oldIndex = pStack->index;

    // Corrupt the collection
    free(pStack->ppCollection);
    pStack->ppCollection = NULL;

    // resize should hard-fail and not touch capacity/index
    if (dynamicStack_resize(pStack, 8) != false)
        return false;
    if (pStack->capacity != oldCap || pStack->index != oldIndex)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_invalidArgs(void)
{
    int val = 42;

    // NULL stack
    if (dynamicStack_push(NULL, &val) != false)
        return false;
    if (dynamicStack_pushUnique(NULL, &val) != false)
        return false;
    if (dynamicStack_pop(NULL) != NULL)
        return false;

    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    // Corrupted/internal ppCollection == NULL should cause ops to fail safely
    free(pStack->ppCollection);
    pStack->ppCollection = NULL;

    if (dynamicStack_push(pStack, &val) != false)
        return false;
    if (dynamicStack_pushUnique(pStack, &val) != false)
        return false;
    if (dynamicStack_pop(pStack) != NULL)
        return false;
    if (dynamicStack_resize(pStack, 8) != false)
        return false;

    // destroy must tolerate ppCollection == NULL
    dynamicStack_destroy(pStack);

    // destroy(NULL) must be a no-op
    dynamicStack_destroy(NULL);

    return true;
}

static bool test_dynamicStack_maxCapacity_enforced(void)
{
    DynamicStack_t *pStack = dynamicStack_create(DYNAMIC_STACK_MAX_CAPACITY);
    if (!pStack)
        return false;

    if (pStack->capacity != DYNAMIC_STACK_MAX_CAPACITY)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    int dummy = 123;

    // Fill exactly to max capacity
    for (size_t i = 0; i < DYNAMIC_STACK_MAX_CAPACITY; ++i)
    {
        if (!dynamicStack_push(pStack, &dummy))
        {
            dynamicStack_destroy(pStack);
            return false; // should be able to fill exactly to max
        }
    }

    if (pStack->index != DYNAMIC_STACK_MAX_CAPACITY)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    size_t oldIndex = pStack->index;
    size_t oldCapacity = pStack->capacity;

    // One more push SHOULD fail, and capacity/index must not change
    if (dynamicStack_push(pStack, &dummy) != false)
    {
        dynamicStack_destroy(pStack);
        return false;
    }
    if (pStack->index != oldIndex || pStack->capacity != oldCapacity)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    // Resize to same capacity is a no-op success
    if (!dynamicStack_resize(pStack, oldCapacity))
    {
        dynamicStack_destroy(pStack);
        return false;
    }
    if (pStack->capacity != oldCapacity || pStack->index != oldIndex)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    // Also test resize beyond max: request a larger size and ensure it fails
    if (dynamicStack_resize(pStack, oldCapacity * 2) != false)
    {
        dynamicStack_destroy(pStack);
        return false;
    }
    if (pStack->capacity != oldCapacity)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_nullData(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    size_t oldIndex = pStack->index;

    if (dynamicStack_push(pStack, NULL) != false)
        return false;
    if (dynamicStack_pushUnique(pStack, NULL) != false)
        return false;

    if (pStack->index != oldIndex)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

static bool test_dynamicStack_scriptedStress(void)
{
    DynamicStack_t *pStack = dynamicStack_create(4);
    if (!pStack)
        return false;

    int a = 1, b = 2, c = 3, d = 4, e = 5;

    // push a, b
    if (!dynamicStack_push(pStack, &a) ||
        !dynamicStack_push(pStack, &b))
        return false;

    // pop -> b
    if (dynamicStack_pop(pStack) != &b)
        return false;

    // pushUnique b, c
    if (!dynamicStack_pushUnique(pStack, &b))
        return false;
    if (!dynamicStack_pushUnique(pStack, &c))
        return false;

    // shrink to current size (index)
    if (!dynamicStack_resize(pStack, pStack->index))
        return false;

    // push d, e to trigger potential grow
    if (!dynamicStack_push(pStack, &d))
        return false;
    if (!dynamicStack_push(pStack, &e))
        return false;

    // Now pop sequence must be: e, d, c, b, a
    if (dynamicStack_pop(pStack) != &e)
        return false;
    if (dynamicStack_pop(pStack) != &d)
        return false;
    if (dynamicStack_pop(pStack) != &c)
        return false;
    if (dynamicStack_pop(pStack) != &b)
        return false;
    if (dynamicStack_pop(pStack) != &a)
        return false;
    if (dynamicStack_pop(pStack) != NULL)
        return false;

    dynamicStack_destroy(pStack);
    return true;
}

int dynamicStack_tests_run(void)
{
    fails += ut_assert(test_dynamicStack_create_and_clamp() == true,
                       "DynamicStack create & clamp");
    fails += ut_assert(test_dynamicStack_push_pop_order() == true,
                       "DynamicStack push/pop LIFO");
    fails += ut_assert(test_dynamicStack_resize_grow_and_shrink() == true,
                       "DynamicStack resize grow/shrink");
    fails += ut_assert(test_dynamicStack_push_resize_preserves_data() == true,
                       "DynamicStack push triggers resize & preserves data");
    fails += ut_assert(test_dynamicStack_pushUnique_behavior() == true,
                       "DynamicStack pushUnique basic");
    fails += ut_assert(test_dynamicStack_pushUnique_pop_readd() == true,
                       "DynamicStack pushUnique + pop re-add");
    fails += ut_assert(test_dynamicStack_shrink_then_grow_via_push() == true,
                       "DynamicStack shrink then grow via push");
    fails += ut_assert(test_dynamicStack_resize_with_null_collection() == true,
                       "DynamicStack resize with NULL collection");
    fails += ut_assert(test_dynamicStack_invalidArgs() == true,
                       "DynamicStack invalid args handling");
    fails += ut_assert(test_dynamicStack_maxCapacity_enforced() == true,
                       "DynamicStack max capacity & resize enforcement");
    fails += ut_assert(test_dynamicStack_nullData() == true,
                       "DynamicStack NULL data handling");
    fails += ut_assert(test_dynamicStack_scriptedStress() == true,
                       "DynamicStack scripted stress");

    return fails;
}
