#pragma region Includes
#pragma once
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

    if (*ppRoot == NULL)
    {
        *ppRoot = pAdd;
        return true;
    }

    LinkedList_t *pCurrent = *ppRoot;
    while (pCurrent->pNext != NULL)
        pCurrent = pCurrent->pNext;

    pCurrent->pNext = pAdd;
    return true;
}

/// @brief Removes the address of pRemove from the list. If returns true, it has been removed and must still be freed by
/// the calling function or otherwise.
static inline bool linkedList_node_remove(LinkedList_t **ppRoot, LinkedList_t *pRemove)
{
    if (!ppRoot || !pRemove || *ppRoot == NULL)
        return false;

    if (*ppRoot == pRemove)
    {
        *ppRoot = pRemove->pNext;
        pRemove->pNext = NULL;
        return true;
    }

    LinkedList_t *pPrevious = *ppRoot;
    LinkedList_t *pCurrent = pPrevious->pNext;
    while (pCurrent->pNext != NULL && pCurrent != pRemove)
    {
        pPrevious = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (!pCurrent)
        return false;

    pPrevious->pNext = pCurrent->pNext;
    pCurrent->pNext = NULL;
    return true;
}

/// @brief Creates a node with pData and adds it to the end of the list
static inline bool linkedList_data_add(LinkedList_t **ppRoot, void *pData)
{
    if (!ppRoot || !pData)
        return false;

    LinkedList_t *pAdd = (LinkedList_t *)malloc(sizeof(LinkedList_t));
    if (!pAdd)
        return false;

    pAdd->pData = pData;
    pAdd->pNext = NULL;

    if (!linkedList_node_add(ppRoot, pAdd))
    {
        free(pAdd);
        return false;
    }

    return true;
}
#pragma endregion