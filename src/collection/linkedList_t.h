#pragma region Includes
#pragma once
#pragma endregion
#pragma region Definitions
typedef struct LinkedList_t
{
    void *pData;
    struct LinkedList_t *pNext;
} LinkedList_t;

/// @brief Callback to destroy/free the payload stored in pData. Pass NULL if the linked list does not own payloads.
typedef void (*LinkedListDataDestructor)(void *pCtx, void *pData);
#pragma endregion
#pragma region Operations
/// @brief Creates a new node with data but doesn't add it to anything
static inline LinkedList_t *linkedList_node_new(void *pData)
{
    LinkedList_t *pNew = malloc(sizeof(LinkedList_t));
    if (!pNew)
        return NULL;

    pNew->pData = pData;
    pNew->pNext = NULL;

    return pNew;
}

/// @brief Creates a root node with data of NULL (sentinel)
static inline LinkedList_t *linkedList_root(void)
{
    return linkedList_node_new(NULL);
}

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

/// @brief Removes the node whose pData equals pData from the list. If returns true,
/// it has been detached from the list and must still be freed by the caller.
static inline bool linkedList_data_remove(LinkedList_t **ppRoot, void *pData)
{
    if (!ppRoot || !pData)
        return false;

    LinkedList_t **pp = ppRoot;
    while (*pp && (*pp)->pData != pData)
        pp = &(*pp)->pNext;

    if (!*pp)
        return false;

    LinkedList_t *pToRemove = *pp;
    *pp = pToRemove->pNext;
    pToRemove->pNext = NULL;
    return true;
}

/// @brief Inserts pAdd after pCurrent (always). pAdd->pNext is set to old pCurrent->pNext.
/// Returns pAdd on success, NULL on bad args.
static inline LinkedList_t *linkedList_node_insertAfter(LinkedList_t *pCurrent, LinkedList_t *pAdd)
{
    if (!pCurrent || !pAdd)
        return NULL;

    pAdd->pNext = pCurrent->pNext;
    pCurrent->pNext = pAdd;

    return pAdd;
}

/// @brief Inserts pAdd after pCurrent (always). pAdd->pNext is set to old pCurrent->pNext.
/// Returns pAdd on success, NULL on bad args.
static inline LinkedList_t *linkedList_data_insertAfter(LinkedList_t *pCurrent, void *pData)
{
    if (!pCurrent || !pData)
        return NULL;

    LinkedList_t *pAdd = linkedList_node_new(pData);
    if (!pAdd)
        return NULL;

    pAdd->pNext = pCurrent->pNext;
    pCurrent->pNext = pAdd;

    return pAdd;
}

/// @brief Creates a node with pData and adds it to the end of the list. Returns the added entry
static inline LinkedList_t *linkedList_data_add(LinkedList_t **ppRoot, void *pData)
{
    if (!ppRoot || !pData)
        return NULL;

    LinkedList_t *pAdd = linkedList_node_new(pData);
    if (!pAdd)
        return NULL;

    if (!linkedList_node_add(ppRoot, pAdd))
    {
        free(pAdd);
        return NULL;
    }

    return pAdd;
}

/// @brief Walks the collection to verify data ADDRESS is unique. If it is, creates a node with pData and adds it to the end
/// of the list. Returns the added entry or the entry that already contains that data.
static inline LinkedList_t *linkedList_data_addUnique(LinkedList_t **ppRoot, void *pData)
{
    if (!ppRoot || !pData)
        return NULL;

    if (*ppRoot == NULL)
    {
        *ppRoot = linkedList_root();
        if (!*ppRoot)
            return NULL;
    }

    LinkedList_t *pTail = *ppRoot;
    for (LinkedList_t *pCurrent = (*ppRoot)->pNext; pCurrent; pCurrent = pCurrent->pNext)
    {
        if (pCurrent->pData == pData)
            return pCurrent;
        pTail = pCurrent;
    }

    return linkedList_data_insertAfter(pTail, pData);
}

/// @brief Walks the collection to verify node ADDRESS is unique. If it is, adds it to the end
/// of the list. Returns the added entry or the entry that already exists.
static inline LinkedList_t *linkedList_node_addUnique(LinkedList_t **ppRoot, LinkedList_t *pAdd)
{
    if (!ppRoot || !pAdd)
        return NULL;

    LinkedList_t *pTail = NULL;
    for (LinkedList_t *pCurrent = *ppRoot; pCurrent; pCurrent = pCurrent->pNext)
    {
        // If the ADDRESS of any node matches pData then it is assumed data would be a duplicate entry. Stop and return the existing
        // collection.
        if (pCurrent == pAdd)
            return pCurrent;

        pTail = pCurrent;
    }

    return linkedList_node_insertAfter(pTail, pAdd);
}

/// @brief Destroys the entire linked list (must be passed root node) and frees node data by calling the passed function on it.
/// Pass destructor function params as a collection through pCtx
static inline bool linkedList_destroy(LinkedList_t **ppRoot, LinkedListDataDestructor destructor, void *pCtx)
{
    size_t removedSize = 0;
    // root node will always have NULL data (sentinel)
    if (!ppRoot)
        return false;

    LinkedList_t *pNode = *ppRoot;
    *ppRoot = NULL;

    while (pNode)
    {
        LinkedList_t *pNext = pNode->pNext;
        if (destructor && pNode->pData)
        {
            destructor(pCtx, pNode->pData);
            removedSize++;
        }

        free(pNode);
        pNode = pNext;
    }

    if (removedSize != 0)
        logs_log(LOG_DEBUG, "Removed LL size: %d", removedSize);

    return true;
}
#pragma endregion