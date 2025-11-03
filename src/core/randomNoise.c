#include "cmath/cmath.h"
#define FNL_USE_DOUBLE 1
#define FNL_IMPL
#include "../lib/FastNoiseLite.h"

#include "world/chunk.h"

static fnl_state fnl_Stone;
// low-frequency domain warper
static fnl_state fnl_StoneWarp;
// controls region size
static const double STONE_SCALE = 0.000007;
// warp field frequency
static const double STONE_WARP_SCALE = 0.00015;
// how strongly to bend space
static const double STONE_WARP_AMPLITUDE = 40.0;
static const double STONE_WARP_OFF_X = 19.19;
static const double STONE_WARP_OFF_Y = 47.47;
static const double STONE_WARP_OFF_Z = 73.73;

/// @brief Initialize the noise system.
void randomNoise_init(uint32_t worldSeed)
{
    fnl_Stone = fnlCreateState();
    fnl_Stone.seed = (int)worldSeed;
    fnl_Stone.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_Stone.frequency = 1.0;
    fnl_Stone.fractal_type = FNL_FRACTAL_FBM;
    fnl_Stone.octaves = 5;
    fnl_Stone.lacunarity = 2.0F;
    fnl_Stone.gain = 0.45F;

    fnl_StoneWarp = fnlCreateState();
    // Decorrelate seed but keeps determinism
    fnl_StoneWarp.seed = (int)(worldSeed ^ 0xA53B1F29U);
    fnl_StoneWarp.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_StoneWarp.frequency = 1.0;
    fnl_StoneWarp.fractal_type = FNL_FRACTAL_FBM;
    fnl_StoneWarp.octaves = 4;
    fnl_StoneWarp.lacunarity = 2.1F;
    fnl_StoneWarp.gain = 0.15F;
}

/// @brief Samples stable domain-warped geological noise at world position.
/// @return Smooth continuous noise value in [-1, 1].
float randomNoise_stoneSampleXYZ(const Chunk_t *pC, int lx, int ly, int lz)
{
    Vec3i_t o = chunkPos_to_worldOrigin(pC->chunkPos);
    const double wx = (double)o.x + (double)lx + 0.5;
    const double wy = (double)o.y + (double)ly + 0.5;
    const double wz = (double)o.z + (double)lz + 0.5;

    // warp field (double)
    const double wpx = fnlGetNoise3D(&fnl_StoneWarp,
                                     (FNLfloat)(wx * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_X,
                                     (FNLfloat)(wy * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_X,
                                     (FNLfloat)(wz * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_X);
    const double wpy = fnlGetNoise3D(&fnl_StoneWarp,
                                     (FNLfloat)(wx * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Y,
                                     (FNLfloat)(wy * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Y,
                                     (FNLfloat)(wz * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Y);
    const double wpz = fnlGetNoise3D(&fnl_StoneWarp,
                                     (FNLfloat)(wx * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Z,
                                     (FNLfloat)(wy * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Z,
                                     (FNLfloat)(wz * STONE_WARP_SCALE) + (FNLfloat)STONE_WARP_OFF_Z);

    const double nx = wx * STONE_SCALE + wpx * STONE_WARP_AMPLITUDE;
    const double ny = wy * STONE_SCALE + wpy * STONE_WARP_AMPLITUDE;
    const double nz = wz * STONE_SCALE + wpz * STONE_WARP_AMPLITUDE;

    return (float)fnlGetNoise3D(&fnl_Stone, (FNLfloat)nx, (FNLfloat)ny, (FNLfloat)nz);
}
