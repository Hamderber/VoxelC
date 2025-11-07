#pragma region Includes
#pragma once
#include "core/logs.h"
#pragma endregion
#pragma region Definitions
typedef struct LinkedList_t
{
    void *pData;
    struct LinkedList_t *pNext;
} LinkedList_t;
#pragma endregion
#pragma region Operations
/// @brief Adds a node to the end of the list
static inline bool linkedList_node_add(LinkedList_t **ppRoot, LinkedList_t *pAdd)
{
    if (!ppRoot || !pAdd)
        return false;

    pAdd->pNext = NULL;

    if (*ppRoot == NULL)
    {
        *ppRoot = pAdd;
        return true;
    }

    LinkedList_t *pCurrent = *ppRoot;
    while (pCurrent && pCurrent->pNext != NULL)
        pCurrent = pCurrent->pNext;

    pCurrent->pNext = pAdd;
    return true;
}

/// @brief Removes the address of pRemove from the list. If returns true, it has been removed and must still be freed by
/// the calling function or otherwise.
static inline bool linkedList_node_remove(LinkedList_t **ppRoot, LinkedList_t *pRemove)
{
    if (!ppRoot || !pRemove)
        return false;

    LinkedList_t **pp = ppRoot;
    while (*pp && *pp != pRemove)
        pp = &(*pp)->pNext;

    if (!*pp)
        return false;

    LinkedList_t *pToRemove = *pp;
    *pp = pToRemove->pNext;
    pToRemove->pNext = NULL;
    return true;
}

/// @brief Creates a node with pData and adds it to the end of the list. Returns the added entry
static inline LinkedList_t *linkedList_data_add(LinkedList_t **ppRoot, void *pData)
{
    if (!ppRoot || !pData)
        return NULL;

    LinkedList_t *pAdd = (LinkedList_t *)malloc(sizeof(LinkedList_t));
    if (!pAdd)
        return NULL;

    pAdd->pData = pData;
    pAdd->pNext = NULL;

    if (!linkedList_node_add(ppRoot, pAdd))
    {
        free(pAdd);
        return NULL;
    }

    return pAdd;
}

#pragma endregion