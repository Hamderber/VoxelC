#include "../../unit_tests.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cmath/cmath.h"
#include "world/voxel/cubeFace_t.h"
#include "world/voxel/block_t.h"
#include "world/voxel/blockFlags.h"

static int fails = 0;

static bool test_face_constants(void)
{
    if (VERTS_PER_FACE != 4)
        return false;
    if (INDICIES_PER_FACE != 6)
        return false;

    return true;
}

static bool test_face_positions_exact(void)
{
    // Expected layout exactly matching your definition, used as a guard
    static const Vec3i_t expected[6][4] = {
        // LEFT (-X)
        [CUBE_FACE_LEFT] = {
            {0, 1, 0},
            {0, 0, 0},
            {0, 1, 1},
            {0, 0, 1},
        },
        // RIGHT (+X)
        [CUBE_FACE_RIGHT] = {
            {1, 1, 1},
            {1, 0, 1},
            {1, 1, 0},
            {1, 0, 0},
        },
        // TOP (+Y)
        [CUBE_FACE_TOP] = {
            {0, 1, 0},
            {0, 1, 1},
            {1, 1, 0},
            {1, 1, 1},
        },
        // BOTTOM (-Y)
        [CUBE_FACE_BOTTOM] = {
            {0, 0, 1},
            {0, 0, 0},
            {1, 0, 1},
            {1, 0, 0},
        },
        // FRONT (+Z)
        [CUBE_FACE_FRONT] = {
            {0, 1, 1},
            {0, 0, 1},
            {1, 1, 1},
            {1, 0, 1},
        },
        // BACK (-Z)
        [CUBE_FACE_BACK] = {
            {1, 1, 0},
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 0},
        },
    };

    if (memcmp(pFACE_POSITIONS, expected, sizeof(expected)) != 0)
        return false;

    return true;
}

static bool test_face_positions_invariants(void)
{
    // Basic sanity: all coords E {0,1}, and each face has a constant axis as expected.
    for (int face = 0; face < 6; ++face)
    {
        for (int v = 0; v < VERTS_PER_FACE; ++v)
        {
            Vec3i_t p = pFACE_POSITIONS[face][v];
            if (p.x < 0 || p.x > 1 ||
                p.y < 0 || p.y > 1 ||
                p.z < 0 || p.z > 1)
                return false;
        }

        switch (face)
        {
        case CUBE_FACE_LEFT:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].x != 0)
                    return false;
            break;
        case CUBE_FACE_RIGHT:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].x != 1)
                    return false;
            break;
        case CUBE_FACE_TOP:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].y != 1)
                    return false;
            break;
        case CUBE_FACE_BOTTOM:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].y != 0)
                    return false;
            break;
        case CUBE_FACE_FRONT:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].z != 1)
                    return false;
            break;
        case CUBE_FACE_BACK:
            for (int v = 0; v < VERTS_PER_FACE; ++v)
                if (pFACE_POSITIONS[face][v].z != 0)
                    return false;
            break;
        default:
            return false;
        }
    }

    return true;
}

static bool test_blockPos_pack_and_index12(void)
{
    // Pick some random-but-valid coordinates and flags
    const uint8_t x = 3;
    const uint8_t y = 9;
    const uint8_t z = 15;
    const uint8_t flagsNibble = 0x0F; // all bits set in nibble

    uint16_t packed = blockPos_pack_localXYZ_flags(x, y, z, flagsNibble);
    uint16_t index12 = blockPosPacked_index12(packed);

    // index12 must match the pure XYZ -> index mapping
    uint16_t expectedIndex = blockPos_pack_localXYZ(x, y, z);
    if (index12 != expectedIndex)
        return false;

    // index must be within [0,4095]
    if (index12 >= 4096)
        return false;

    // All four flag bits should read as set
    if (!blockPosPacked_flag_get(packed, BLOCKPOS_PACKED_FLAG_AIR))
        return false;
    if (!blockPosPacked_flag_get(packed, BLOCKPOS_PACKED_FLAG_LIQUID))
        return false;
    if (!blockPosPacked_flag_get(packed, BLOCKPOS_PACKED_FLAG_RESERVED_1))
        return false;
    if (!blockPosPacked_flag_get(packed, BLOCKPOS_PACKED_FLAG_RESERVED_2))
        return false;

    return true;
}

static bool test_blockPos_flag_set_clear_idempotent(void)
{
    const uint8_t x = 5, y = 7, z = 11;
    uint16_t base = blockPos_pack_localXYZ_flags(x, y, z, 0);

    // Initially all flags off
    for (int f = BLOCKPOS_PACKED_FLAG_AIR; f <= BLOCKPOS_PACKED_FLAG_RESERVED_2; ++f)
        if (blockPosPacked_flag_get(base, (BlockPosPackedFlag_e)f))
            return false;

    // Set AIR, check; clearing preserves index.
    uint16_t withAir = blockPosPacked_flag_set(base, BLOCKPOS_PACKED_FLAG_AIR);
    if (!blockPosPacked_flag_get(withAir, BLOCKPOS_PACKED_FLAG_AIR))
        return false;
    if (blockPosPacked_index12(withAir) != blockPosPacked_index12(base))
        return false;

    // Clear AIR, back to original flags (and same index)
    uint16_t cleared = blockPosPacked_flag_clear(withAir, BLOCKPOS_PACKED_FLAG_AIR);
    if (blockPosPacked_flag_get(cleared, BLOCKPOS_PACKED_FLAG_AIR))
        return false;
    if (blockPosPacked_index12(cleared) != blockPosPacked_index12(base))
        return false;

    // Clearing a flag that is already clear should be idempotent
    uint16_t clearedAgain = blockPosPacked_flag_clear(cleared, BLOCKPOS_PACKED_FLAG_LIQUID);
    if (clearedAgain != cleared)
        return false;

    return true;
}

static bool test_block_isSolid_semantics(void)
{
    const uint8_t x = 1, y = 2, z = 3;

    // No flags set => implicit solid
    uint16_t solidPacked = blockPos_pack_localXYZ_flags(x, y, z, 0x0);
    if (!block_isSolid(solidPacked))
        return false;

    // Mark AIR => not solid
    uint16_t airPacked = blockPosPacked_flag_set(solidPacked, BLOCKPOS_PACKED_FLAG_AIR);
    if (block_isSolid(airPacked))
        return false;

    // Mark LIQUID in addition to AIR => still not solid
    uint16_t airLiquidPacked = blockPosPacked_flag_set(airPacked, BLOCKPOS_PACKED_FLAG_LIQUID);
    if (block_isSolid(airLiquidPacked))
        return false;

    // Clear AIR but leave LIQUID => becomes solid again per current semantics (!AIR = solid)
    uint16_t justLiquid = blockPosPacked_flag_clear(airLiquidPacked, BLOCKPOS_PACKED_FLAG_AIR);
    if (!block_isSolid(justLiquid))
        return false;

    return true;
}

static bool test_blockDef_isTransparent(void)
{
    // Air should be transparent
    if (!blockDef_isTransparent(&sBLOCK_DEF_AIR))
        return false;

    // A typical stone block should not be transparent
    if (blockDef_isTransparent(&sBLOCK_DEF_STONE))
        return false;

    // NULL should be safely false
    if (blockDef_isTransparent(NULL))
        return false;

    return true;
}

int voxel_tests_run(void)
{
    fails += ut_assert(test_face_constants() == true,
                       "Voxel: face constants (VERTS_PER_FACE / INDICIES_PER_FACE)");
    fails += ut_assert(test_face_positions_exact() == true,
                       "Voxel: pFACE_POSITIONS exact layout");
    fails += ut_assert(test_face_positions_invariants() == true,
                       "Voxel: pFACE_POSITIONS invariants");

    fails += ut_assert(test_blockPos_pack_and_index12() == true,
                       "Voxel: blockPos_pack_localXYZ_flags + blockPosPacked_index12");
    fails += ut_assert(test_blockPos_flag_set_clear_idempotent() == true,
                       "Voxel: blockPosPacked_flag_{set,clear,get} behavior");
    fails += ut_assert(test_block_isSolid_semantics() == true,
                       "Voxel: block_isSolid AIR/LIQUID semantics");

    fails += ut_assert(test_blockDef_isTransparent() == true,
                       "Voxel: blockDef_isTransparent basic behavior");

    return fails;
}
