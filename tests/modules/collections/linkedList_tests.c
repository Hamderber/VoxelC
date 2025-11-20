#include <stdbool.h>
#include "collection/linkedList_t.h"
#include "../../unit_tests.h"

static int fails = 0;

static bool test_linkedList_node_new_and_root(void)
{
    int value = 42;
    LinkedList_t *pNode = linkedList_node_new(&value);
    if (!pNode)
        return false;

    if (pNode->pData != &value || pNode->pNext != NULL)
    {
        free(pNode);
        return false;
    }

    LinkedList_t *pRoot = linkedList_create();
    if (!pRoot)
    {
        free(pNode);
        return false;
    }

    if (pRoot->pData != NULL || pRoot->pNext != NULL)
    {
        free(pNode);
        free(pRoot);
        return false;
    }

    free(pNode);
    free(pRoot);
    return true;
}

static bool test_linkedList_node_add_and_data_add(bool invalidRoot)
{
    LinkedList_t *pRoot = NULL;
    if (!invalidRoot)
        pRoot = linkedList_create();

    int a = 1;
    int b = 2;
    int c = 3;

    LinkedList_t *pN1 = linkedList_data_add(&pRoot, &a);
    if (!pN1 || pRoot->pNext != pN1 || pN1->pData != &a || pN1->pNext != NULL)
        return false;

    LinkedList_t *pN2 = linkedList_data_add(&pRoot, &b);
    if (!pN2 || pN2->pData != &b || pN1->pNext != pN2 || pN2->pNext != NULL)
        return false;

    LinkedList_t *pN3 = linkedList_node_new(&c);
    if (!pN3)
        return false;

    if (!linkedList_node_add(&pRoot, pN3))
        return false;

    if (pN2->pNext != pN3 || pN3->pNext != NULL)
        return false;

    // traversal order root -> a -> b -> c
    if (pRoot->pNext->pData != &a ||
        pRoot->pNext->pNext->pData != &b ||
        pRoot->pNext->pNext->pNext->pData != &c)
        return false;

    return linkedList_destroy(&pRoot, NULL, NULL);
}

static bool test_linkedList_node_remove(void)
{
    // root -> a -> b -> c
    int a = 1, b = 2, c = 3;
    LinkedList_t *pRoot = linkedList_create();

    LinkedList_t *pN1 = linkedList_data_add(&pRoot, &a);
    LinkedList_t *pN2 = linkedList_data_add(&pRoot, &b);
    LinkedList_t *pN3 = linkedList_data_add(&pRoot, &c);
    if (!pN1 || !pN2 || !pN3)
        return false;

    // Remove middle (b)
    if (!linkedList_node_remove(&pRoot, pN2))
        return false;

    if (pN2->pNext != NULL)
        return false;
    // a -> c
    if (pRoot->pNext != pN1 || pN1->pNext != pN3)
        return false;

    // Remove head (a) (not root)
    if (!linkedList_node_remove(&pRoot, pN1))
        return false;

    if (pN1->pNext != NULL)
        return false;
    // only c remains
    if (pRoot->pNext != pN3 || pN3->pNext != NULL)
        return false;

    // Remove non-existing node
    LinkedList_t stray = {.pData = &a, .pNext = NULL};
    if (linkedList_node_remove(&pRoot, &stray))
        return false;

    return linkedList_destroy(&pRoot, NULL, NULL);
}

static bool test_linkedList_data_remove(void)
{
    int a = 1, b = 2, c = 3;
    LinkedList_t *pRoot = linkedList_create();

    LinkedList_t *pN1 = linkedList_data_add(&pRoot, &a);
    LinkedList_t *pN2 = linkedList_data_add(&pRoot, &b);
    LinkedList_t *pN3 = linkedList_data_add(&pRoot, &c);
    if (!pN1 || !pN2 || !pN3)
        return false;

    // Remove by data (middle element)
    if (!linkedList_data_remove(&pRoot, &b))
        return false;

    if (pRoot->pNext != pN1 || pN1->pNext != pN3 || pN3->pNext != NULL)
        return false;

    // Remove by data (head)
    if (!linkedList_data_remove(&pRoot, &a))
        return false;

    if (pRoot->pNext != pN3 || pN3->pNext != NULL)
        return false;

    // Remove by data (last element)
    if (!linkedList_data_remove(&pRoot, &c))
        return false;

    if (pRoot->pNext)
        return false;

    // Remove with non-existing data
    if (linkedList_data_remove(&pRoot, &a))
        return false;

    // Remove with NULL data should fail
    if (linkedList_data_remove(&pRoot, NULL))
        return false;

    free(pN1);
    free(pN2);
    free(pN3);

    return pRoot;
}

static bool test_linkedList_insertAfter(void)
{
    int a = 1, b = 2, c = 3;

    LinkedList_t *pN1 = linkedList_node_new(&a);
    if (!pN1)
        return false;

    LinkedList_t *pN2 = linkedList_node_new(&b);
    if (!pN2)
    {
        free(pN1);
        return false;
    }

    LinkedList_t *pN3 = linkedList_node_new(&c);
    if (!pN3)
    {
        free(pN1);
        free(pN2);
        return false;
    }

    LinkedList_t *pRoot = pN1;

    // Insert pN2 after pN1
    if (!linkedList_node_insertAfter(pN1, pN2))
        return false;

    if (pRoot != pN1 || pN1->pNext != pN2 || pN2->pNext != NULL)
        return false;

    // data_insertAfter (creates new node) after pN2
    LinkedList_t *pN3_inserted = linkedList_data_insertAfter(pN2, &c);
    if (!pN3_inserted)
        return false;

    if (pN2->pNext != pN3_inserted || pN3_inserted->pNext != NULL)
        return false;

    if (pN3_inserted->pData != &c)
        return false;

    // Invalid args return NULL
    if (linkedList_node_insertAfter(NULL, pN3) != NULL)
        return false;
    if (linkedList_data_insertAfter(NULL, &a) != NULL)
        return false;
    if (linkedList_data_insertAfter(pN1, NULL) != NULL)
        return false;

    // cleanup
    free(pN1);
    free(pN2);
    free(pN3);
    free(pN3_inserted);
    return true;
}

static bool test_linkedList_data_addUnique(void)
{
    LinkedList_t *pRoot = linkedList_create();

    int a = 1, b = 2;

    // First addUnique should create sentinel root and first data node
    LinkedList_t *pNA1 = linkedList_data_addUnique(&pRoot, &a);
    if (!pNA1 || !pRoot)
        return false;

    if (pRoot->pNext != pNA1)
        return false;
    if (pNA1->pData != &a || pNA1->pNext != NULL)
        return false;

    // Second addUnique with same data should return same node, no new node
    LinkedList_t *pNA2 = linkedList_data_addUnique(&pRoot, &a);
    if (!pNA2 || pNA2 != pNA1)
        return false;
    if (pNA1->pNext != NULL)
        return false;

    // Add different data -> new node at end
    LinkedList_t *pNB = linkedList_data_addUnique(&pRoot, &b);
    if (!pNB || pNB == pNA1)
        return false;

    if (pNA1->pNext != pNB || pNB->pNext != NULL)
        return false;
    if (pNB->pData != &b)
        return false;

    // Test invalid args
    if (linkedList_data_addUnique(NULL, &a) != NULL)
        return false;
    if (linkedList_data_addUnique(&pRoot, NULL) != NULL)
        return false;

    return linkedList_destroy(&pRoot, NULL, NULL);
}

static bool test_linkedList_node_addUnique(void)
{
    LinkedList_t *pRoot = linkedList_create();
    if (!pRoot)
        return false;

    int a = 1, b = 2;
    LinkedList_t *pNA = linkedList_node_new(&a);
    LinkedList_t *pNB = linkedList_node_new(&b);
    if (!pNA || !pNB)
        return false;

    // First addUnique should append after root
    LinkedList_t *rA1 = linkedList_node_addUnique(&pRoot, pNA);
    if (!rA1 || rA1 != pNA)
        return false;
    if (pRoot->pNext != pNA || pNA->pNext != NULL)
        return false;

    // Re-adding same node should return existing, not relink it
    LinkedList_t *rA2 = linkedList_node_addUnique(&pRoot, pNA);
    if (!rA2 || rA2 != pNA)
        return false;
    if (pRoot->pNext != pNA || pNA->pNext != NULL)
        return false;

    // Add another node; should append after nA
    LinkedList_t *rB = linkedList_node_addUnique(&pRoot, pNB);
    if (!rB || rB != pNB)
        return false;
    if (pNA->pNext != pNB || pNB->pNext != NULL)
        return false;

    // Invalid args
    if (linkedList_node_addUnique(NULL, pNA) != NULL)
        return false;
    if (linkedList_node_addUnique(&pRoot, NULL) != NULL)
        return false;

    // cleanup
    linkedList_destroy(&pRoot, NULL, NULL);
    return true;
}

typedef struct DestructorTestCtx_t
{
    int destroyedCount;
} DestructorTestCtx_t;

static void destructor_count(void *pCtx, void *pData)
{
    (void)pData;
    DestructorTestCtx_t *pDestCtx = (DestructorTestCtx_t *)pCtx;
    if (pDestCtx)
        pDestCtx->destroyedCount++;
}

static bool test_linkedList_destroy(void)
{
    // Destroy NULL root pointer
    if (linkedList_destroy(NULL, NULL, NULL) != false)
        return false;

    // Invalid list (*ppRoot == NULL)
    LinkedList_t *pRootEmpty = NULL;
    if (linkedList_destroy(&pRootEmpty, NULL, NULL) != true)
        return false;
    if (pRootEmpty)
        return false;

    // Case 3: list with some nodes and destructor
    LinkedList_t *pRoot = linkedList_create();
    if (!pRoot)
        return false;

    int a = 1, b = 2, c = 3;
    LinkedList_t *pNA = linkedList_data_add(&pRoot, &a);
    LinkedList_t *pNB = linkedList_data_add(&pRoot, &b);
    LinkedList_t *pNC = linkedList_data_add(&pRoot, &c);
    if (!pNA || !pNB || !pNC)
        return false;

    DestructorTestCtx_t pCtx = {.destroyedCount = 0};

    if (!linkedList_destroy(&pRoot, destructor_count, &pCtx))
        return false;

    if (pRoot != NULL)
        return false;

    // Root sentinel had NULL data, so only 3 payloads should be counted
    if (pCtx.destroyedCount != 3)
        return false;

    return true;
}

int linkedLists_tests_run(void)
{
    fails += ut_assert(test_linkedList_node_new_and_root() == true,
                       "LinkedList node_new and root");
    fails += ut_assert(test_linkedList_node_add_and_data_add(true) == false,
                       "LinkedList node_add and data_add (invalid root)");
    fails += ut_assert(test_linkedList_node_add_and_data_add(false) == true,
                       "LinkedList node_add and data_add");
    fails += ut_assert(test_linkedList_node_remove() == true,
                       "LinkedList node_remove");
    fails += ut_assert(test_linkedList_data_remove() == true,
                       "LinkedList data_remove");
    fails += ut_assert(test_linkedList_insertAfter() == true,
                       "LinkedList insertAfter");
    fails += ut_assert(test_linkedList_data_addUnique() == true,
                       "LinkedList data_addUnique");
    fails += ut_assert(test_linkedList_node_addUnique() == true,
                       "LinkedList node_addUnique");
    fails += ut_assert(test_linkedList_destroy() == true,
                       "LinkedList destroy");

    return fails;
}