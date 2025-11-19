#pragma region Includes
#include "cmath.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Definitions
#if defined(DEBUG)
// #define DEBUG_CMATH
#endif
typedef enum CrashType_e
{
    CRASH_CHUNK_MATH,
} CrashType_e;
#pragma endregion
#pragma region Crash
static void crash(CrashType_e crashType)
{
    switch (crashType)
    {
    case CRASH_CHUNK_MATH:
        logs_log(LOG_ERROR, "Failed to bake chunk math!");
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without usable pre-baked expensive math operations.");
        break;
    }
}
#pragma endregion
#pragma region Chunk
Vec3i_t *cmath_chunk_chunkNeighborPos_get(const Vec3i_t CHUNK_POS)
{
    Vec3i_t *pPos = malloc(sizeof(Vec3i_t) * CMATH_GEOM_CUBE_FACES);
    if (!pPos)
        return NULL;

    for (int i = 0; i < CMATH_GEOM_CUBE_FACES; ++i)
        pPos[i] = cmath_vec3i_add_vec3i(CHUNK_POS, pCMATH_CUBE_NEIGHBOR_OFFSETS[i]);

    return pPos;
}

Vec3i_t *cmath_chunk_GetNeighborsPosUnique_get(const Vec3i_t *restrict pCHUNK_POS, const size_t NUM_POS, size_t *restrict pOutCount)
{
    if (!pCHUNK_POS || !pOutCount || NUM_POS <= 0)
        return NULL;

    *pOutCount = 0;

    Vec3i_t *pPos = malloc(sizeof(Vec3i_t) * CMATH_GEOM_CUBE_FACES * NUM_POS);
    if (!pPos)
        return NULL;

    const int TOLERANCE = 0;
    for (size_t i = 0; i < NUM_POS; ++i)
    {
        const Vec3i_t CHUNK_POS = pCHUNK_POS[i];
        for (uint8_t face = 0; face < CMATH_GEOM_CUBE_FACES; face++)
        {
            Vec3i_t neighborPos = cmath_vec3i_add_vec3i(CHUNK_POS, pCMATH_CUBE_NEIGHBOR_OFFSETS[face]);

            bool duplicate = false;
            for (size_t j = 0; j < NUM_POS; j++)
            {
                if (cmath_vec3i_equals(neighborPos, pCHUNK_POS[j], TOLERANCE))
                {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
                for (size_t j = 0; j < *pOutCount; j++)
                {
                    if (cmath_vec3i_equals(neighborPos, pPos[j], TOLERANCE))
                    {
                        duplicate = true;
                        break;
                    }
                }

            if (!duplicate)
            {
                pPos[*pOutCount] = neighborPos;
                (*pOutCount)++;
            }
        }
    }

    return pPos;
}

static Vec3u8_t *pCMATH_CHUNK_POINTS = NULL;
Vec3u8_t *cmath_chunkPoints_Get(void) { return pCMATH_CHUNK_POINTS; };
static uint16_t *pCMATH_CHUNK_POINTS_PACKED = NULL;
uint16_t *cmath_chunkPointsPacked_Get(void) { return pCMATH_CHUNK_POINTS_PACKED; };

static Vec3u8_t *pCMATH_CHUNK_CORNER_POINTS = NULL;
Vec3u8_t *cmath_chunkCornerPoints_Get(void) { return pCMATH_CHUNK_CORNER_POINTS; };

static Vec3u8_t *pCMATH_CHUNK_SHELL_EDGE_POINTS = NULL;
Vec3u8_t *cmath_chunkShellEdgePoints_Get(void) { return pCMATH_CHUNK_SHELL_EDGE_POINTS; };

static Vec3u8_t *pCMATH_CHUNK_INNER_POINTS = NULL;
Vec3u8_t *cmath_chunkInnerPoints_Get(void) { return pCMATH_CHUNK_INNER_POINTS; };

static Vec3u8_t *pCMATH_CHUNK_SHELL_BORDERLESS_POINTS = NULL;
Vec3u8_t *cmath_chunkShellBorderlessPoints_Get(void) { return pCMATH_CHUNK_SHELL_BORDERLESS_POINTS; };

static Vec3u8_t *pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS = NULL;
Vec3u8_t *cmath_chunk_blockNeighborPoints_Get(void) { return pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS; }

static bool *pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL = NULL;
bool *cmath_chunk_blockNeighborPointsInChunkBool_Get(void) { return pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL; };

/// @brief Bake the array of positions in the chunk both packed (uint16_t) and local space (Vec3u8_t).
static void bake_chunkPoints(void)
{
    // An or here works because they are baked at the same time
    if (pCMATH_CHUNK_POINTS && pCMATH_CHUNK_POINTS_PACKED)
        return;

    if (!pCMATH_CHUNK_POINTS)
        pCMATH_CHUNK_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_POINTS_COUNT);

    if (!pCMATH_CHUNK_POINTS)
    {
        free(pCMATH_CHUNK_POINTS);
        pCMATH_CHUNK_POINTS = NULL;
        free(pCMATH_CHUNK_POINTS_PACKED);
        pCMATH_CHUNK_POINTS_PACKED = NULL;
        crash(CRASH_CHUNK_MATH);
        return;
    }

    if (!pCMATH_CHUNK_POINTS_PACKED)
        pCMATH_CHUNK_POINTS_PACKED = malloc(sizeof(uint16_t) * CMATH_CHUNK_POINTS_PACKED_COUNT);

    if (!pCMATH_CHUNK_POINTS_PACKED)
    {
        free(pCMATH_CHUNK_POINTS);
        pCMATH_CHUNK_POINTS = NULL;
        free(pCMATH_CHUNK_POINTS_PACKED);
        pCMATH_CHUNK_POINTS_PACKED = NULL;
        crash(CRASH_CHUNK_MATH);
        return;
    }

    const uint8_t AXIS_MIN = 0, AXIS_MAX = (uint8_t)CMATH_CHUNK_AXIS_LENGTH;

    for (uint8_t x = AXIS_MIN; x < AXIS_MAX; x++)
        for (uint8_t y = AXIS_MIN; y < AXIS_MAX; y++)
            for (uint8_t z = AXIS_MIN; z < AXIS_MAX; z++)
            {
                uint16_t index = xyz_to_chunkBlockIndex(x, y, z);
                pCMATH_CHUNK_POINTS[index] = (Vec3u8_t){x, y, z};
                pCMATH_CHUNK_POINTS_PACKED[index] = blockPos_pack_localXYZ(x, y, z);
            }
}

/// @brief Bake the array of positions in the chunk shell edges. The outer edges of the chunk minus corners. The inner faces of the shell are skipped (Vec3u8_t).
static void bake_chunkShellEdgePoints(void)
{
    if (pCMATH_CHUNK_SHELL_EDGE_POINTS)
        return;

    if (!pCMATH_CHUNK_SHELL_EDGE_POINTS)
        pCMATH_CHUNK_SHELL_EDGE_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_SHELL_EDGE_POINTS_COUNT);

    if (!pCMATH_CHUNK_SHELL_EDGE_POINTS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    const uint8_t AXIS_MIN = 0, AXIS_MAX = (uint8_t)CMATH_CHUNK_AXIS_LENGTH - 1;

    size_t index = 0;
    // vertical edges (exclude vertices)
    for (uint8_t y = AXIS_MIN + 1; y <= AXIS_MAX - 1; ++y)
    {
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MIN, y, AXIS_MIN};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MIN, y, AXIS_MAX};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MAX, y, AXIS_MIN};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MAX, y, AXIS_MAX};
    }

    // 4 top edges (exclude corners)
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
    {
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){x, AXIS_MAX, AXIS_MIN};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){x, AXIS_MAX, AXIS_MAX};
    }
    for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
    {
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MIN, AXIS_MAX, z};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MAX, AXIS_MAX, z};
    }

    // 4 bottom edges (exclude corners)
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
    {
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){x, AXIS_MIN, AXIS_MIN};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){x, AXIS_MIN, AXIS_MAX};
    }
    for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
    {
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MIN, AXIS_MIN, z};
        pCMATH_CHUNK_SHELL_EDGE_POINTS[index++] = (Vec3u8_t){AXIS_MAX, AXIS_MIN, z};
    }
}

/// @brief Bake the array of positions in the chunk shell without borders (Vec3u8_t).
static void bake_chunkShellBorderless(void)
{
    if (pCMATH_CHUNK_SHELL_BORDERLESS_POINTS)
        return;

    pCMATH_CHUNK_SHELL_BORDERLESS_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_SHELL_BORDERLESS_POINTS_COUNT);

    if (!pCMATH_CHUNK_SHELL_BORDERLESS_POINTS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    const uint8_t AXIS_MIN = 0, AXIS_MAX = (uint8_t)CMATH_CHUNK_AXIS_LENGTH - 1;
    size_t index = 0;

    // y = MAX (top), interior in x/z
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
        for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){x, AXIS_MAX, z};

    // y = MIN (bottom), interior in x/z
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
        for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){x, AXIS_MIN, z};

    // z = MAX (front), interior in x/y
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
        for (uint8_t y = AXIS_MIN + 1; y <= AXIS_MAX - 1; ++y)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){x, y, AXIS_MAX};

    // z = MIN (back), interior in x/y
    for (uint8_t x = AXIS_MIN + 1; x <= AXIS_MAX - 1; ++x)
        for (uint8_t y = AXIS_MIN + 1; y <= AXIS_MAX - 1; ++y)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){x, y, AXIS_MIN};

    // x = MAX (right), interior in y/z
    for (uint8_t y = AXIS_MIN + 1; y <= AXIS_MAX - 1; ++y)
        for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){AXIS_MAX, y, z};

    // x = MIN (left), interior in y/z
    for (uint8_t y = AXIS_MIN + 1; y <= AXIS_MAX - 1; ++y)
        for (uint8_t z = AXIS_MIN + 1; z <= AXIS_MAX - 1; ++z)
            pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[index++] = (Vec3u8_t){AXIS_MIN, y, z};
}

/// @brief Bake the array of chunk shell corners (Vec3u8_t).
static void bake_chunkCornerPoints(void)
{
    if (pCMATH_CHUNK_CORNER_POINTS)
        return;

    if (!pCMATH_CHUNK_CORNER_POINTS)
        pCMATH_CHUNK_CORNER_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_CORNER_POINTs_COUNT);

    if (!pCMATH_CHUNK_CORNER_POINTS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    const uint8_t AXIS_MIN = 0;
    const uint8_t AXIS_MAX = (uint8_t)CMATH_CHUNK_AXIS_LENGTH - 1;

    pCMATH_CHUNK_CORNER_POINTS[0] = (Vec3u8_t){AXIS_MAX, AXIS_MAX, AXIS_MAX};
    pCMATH_CHUNK_CORNER_POINTS[1] = (Vec3u8_t){AXIS_MAX, AXIS_MAX, AXIS_MIN};
    pCMATH_CHUNK_CORNER_POINTS[2] = (Vec3u8_t){AXIS_MAX, AXIS_MIN, AXIS_MAX};
    pCMATH_CHUNK_CORNER_POINTS[3] = (Vec3u8_t){AXIS_MAX, AXIS_MIN, AXIS_MIN};
    pCMATH_CHUNK_CORNER_POINTS[4] = (Vec3u8_t){AXIS_MIN, AXIS_MAX, AXIS_MAX};
    pCMATH_CHUNK_CORNER_POINTS[5] = (Vec3u8_t){AXIS_MIN, AXIS_MAX, AXIS_MIN};
    pCMATH_CHUNK_CORNER_POINTS[6] = (Vec3u8_t){AXIS_MIN, AXIS_MIN, AXIS_MAX};
    pCMATH_CHUNK_CORNER_POINTS[7] = (Vec3u8_t){AXIS_MIN, AXIS_MIN, AXIS_MIN};
}

/// @brief Bake the array of positions in the chunk minus shell edges (Vec3u8_t).
static void bake_chunkInnerPoints(void)
{
    if (pCMATH_CHUNK_INNER_POINTS)
        return;

    if (!pCMATH_CHUNK_INNER_POINTS)
        pCMATH_CHUNK_INNER_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_INNER_POINTS_COUNT);

    if (!pCMATH_CHUNK_INNER_POINTS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    const uint8_t AXIS_MIN = 1, AXIS_MAX = (uint8_t)CMATH_CHUNK_AXIS_LENGTH - 1;

    size_t index = 0;
    for (uint8_t x = AXIS_MIN; x < AXIS_MAX; x++)
        for (uint8_t y = AXIS_MIN; y < AXIS_MAX; y++)
            for (uint8_t z = AXIS_MIN; z < AXIS_MAX; z++)
                pCMATH_CHUNK_INNER_POINTS[index++] = (Vec3u8_t){x, y, z};

    // for (size_t i = 0; i < CMATH_CHUNK_SHELL_BORDERLESS_POINTS_COUNT; i++)
    //     pCMATH_CHUNK_INNER_POINTS[index++] = pCMATH_CHUNK_SHELL_BORDERLESS_POINTS[i];
}

/// @brief Bake a chunk's packed neighbors. Wraps into neighbor chunks.
static void bake_blockNeighborPos(void)
{
    const Vec3u8_t *pPOINTS = cmath_chunkPoints_Get();
    if (!pPOINTS || !pCMATH_CUBE_NEIGHBOR_OFFSETS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS = malloc(sizeof(Vec3u8_t) * CMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_COUNT);
    if (!pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL = malloc(sizeof(bool) * CMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL_COUNT);
    if (!pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL)
    {
        crash(CRASH_CHUNK_MATH);
        return;
    }

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; ++i)
    {
        const uint8_t X = pPOINTS[i].x;
        const uint8_t Y = pPOINTS[i].y;
        const uint8_t Z = pPOINTS[i].z;

        for (int face = 0; face < 6; ++face)
        {
            const Vec3i_t N_POS = cmath_vec3i_add_vec3i(pCMATH_CUBE_NEIGHBOR_OFFSETS[face], (Vec3i_t){X, Y, Z});

            uint8_t NX = (uint8_t)N_POS.x;
            uint8_t NY = (uint8_t)N_POS.y;
            uint8_t NZ = (uint8_t)N_POS.z;

            const bool IN_CHUNK = (NX >= 0 && NX < (int)CMATH_CHUNK_AXIS_LENGTH) &&
                                  (NY >= 0 && NY < (int)CMATH_CHUNK_AXIS_LENGTH) &&
                                  (NZ >= 0 && NZ < (int)CMATH_CHUNK_AXIS_LENGTH);

            const Vec3u8_t POS = (Vec3u8_t){NX, NY, NZ};

            pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS[cmath_blockNeighborIndex(i, face)] =
                IN_CHUNK
                    ? POS
                    : cmath_chunk_wrapLocalPos(POS.x, POS.y, POS.z, face);

            pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL[cmath_blockNeighborIndex(i, face)] = IN_CHUNK;
        }
    }
}
#pragma endregion
#if defined(DEBUG_CMATH)
#pragma region Validation
static bool validation_localPointsInChunk(const Vec3u8_t *pPOINTS, size_t size, bool verbose, const char *pName)
{
    if (!pPOINTS || !pName)
        return false;

    Vec3u8_t *pWrongPoints = malloc(sizeof(Vec3u8_t) * size);
    if (!pWrongPoints)
        return false;

    size_t wrongCount = 0;
    for (size_t i = 0; i < size; i++)
    {
        const Vec3u8_t POINT = pPOINTS[i];
        const uint8_t BOUND = (uint8_t)(CMATH_CHUNK_AXIS_LENGTH - 1);
        if (POINT.x > BOUND || POINT.y > BOUND || POINT.z > BOUND)
            pWrongPoints[wrongCount++] = POINT;
    }

    if (wrongCount > 0)
    {
        logs_log(LOG_ERROR, "%s: %zu local points are baked incorrectly! (Outside of chunk)", pName, wrongCount);
        if (verbose)
        {
            for (size_t i = 0; i < wrongCount; i++)
            {
                const Vec3u8_t POINT = pWrongPoints[i];
                logs_log(LOG_ERROR, "Point (%u, %u, %u) is out of chunk bounds!", POINT.x, POINT.y, POINT.z);
            }
        }
    }

    free(pWrongPoints);
    return wrongCount == 0;
}
#pragma endregion
#endif
#pragma region Instantiate
static bool instantiated = false;
void cmath_instantiate(void)
{
    if (!instantiated)
    {
        bake_chunkPoints();
        bake_chunkShellEdgePoints();
        bake_chunkCornerPoints();
        bake_chunkShellBorderless();
        bake_chunkInnerPoints();
        bake_blockNeighborPos();
        instantiated = true;
    }
    else
        logs_log(LOG_ERROR, "Attempted to double-initialize cmath!");

#if defined(DEBUG_CMATH)
    const bool VERBOSE = true;
    logs_log(LOG_DEBUG, "Cmath validation in progress...");
    validation_localPointsInChunk(pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS, CMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS");
    validation_localPointsInChunk(pCMATH_CHUNK_CORNER_POINTS, CMATH_CHUNK_CORNER_POINTs_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_CORNER_POINTS");
    validation_localPointsInChunk(pCMATH_CHUNK_INNER_POINTS, CMATH_CHUNK_INNER_POINTS_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_INNER_POINTS");
    validation_localPointsInChunk(pCMATH_CHUNK_POINTS, CMATH_CHUNK_POINTS_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_POINTS");
    validation_localPointsInChunk(pCMATH_CHUNK_SHELL_BORDERLESS_POINTS, CMATH_CHUNK_SHELL_BORDERLESS_POINTS_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_SHELL_BORDERLESS_POINTS");
    validation_localPointsInChunk(pCMATH_CHUNK_SHELL_EDGE_POINTS, CMATH_CHUNK_SHELL_EDGE_POINTS_COUNT,
                                  VERBOSE, "pCMATH_CHUNK_SHELL_EDGE_POINTS");
    logs_log(LOG_DEBUG, "Cmath validation complete.");
#endif
}
#pragma endregion
#pragma region Destroy
void cmath_destroy(void)
{
    free(pCMATH_CHUNK_SHELL_EDGE_POINTS);
    pCMATH_CHUNK_SHELL_EDGE_POINTS = NULL;
    free(pCMATH_CHUNK_CORNER_POINTS);
    pCMATH_CHUNK_CORNER_POINTS = NULL;
    free(pCMATH_CHUNK_SHELL_BORDERLESS_POINTS);
    pCMATH_CHUNK_SHELL_BORDERLESS_POINTS = NULL;
    free(pCMATH_CHUNK_INNER_POINTS);
    pCMATH_CHUNK_INNER_POINTS = NULL;
    free(pCMATH_CHUNK_POINTS);
    pCMATH_CHUNK_POINTS = NULL;
    free(pCMATH_CHUNK_POINTS_PACKED);
    pCMATH_CHUNK_POINTS_PACKED = NULL;
    free(pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL);
    pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_IN_CHUNK_BOOL = NULL;
    free(pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS);
    pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS = NULL;

    instantiated = false;
}
#pragma endregion