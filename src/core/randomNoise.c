#pragma region Includes
#include "cmath/cmath.h"
#define FNL_USE_DOUBLE 1
#define FNL_IMPL
#include "../lib/FastNoiseLite.h"
#pragma endregion
#pragma region Stone
static fnl_state fnl_Stone;
static fnl_state fnl_StoneWarp;
static const double STONE_SCALE = 0.000014;
static const double STONE_WARP_SCALE = 0.00015;
static const double STONE_WARP_AMPLITUDE = 40.0;
static const double STONE_WARP_OFF_X = 19.19;
static const double STONE_WARP_OFF_Y = 47.47;
static const double STONE_WARP_OFF_Z = 73.73;

/// @brief Samples stable domain-warped geological noise at world position.
/// @return Smooth continuous noise value in [-1, 1].
float randomNoise_stone_samplePackedPos(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3f_t ORIGIN = cmath_chunk_blockPosPacked_2_worldSamplePos(CHUNK_POS, BLOCK_POS_PACKED12);
    const double WX = (double)ORIGIN.x;
    const double WY = (double)ORIGIN.y;
    const double WZ = (double)ORIGIN.z;

    // warp field (double)
    const double WARP_X = fnlGetNoise3D(&fnl_StoneWarp,
                                        (FNLfloat)(WX * STONE_WARP_SCALE + STONE_WARP_OFF_X),
                                        (FNLfloat)(WY * STONE_WARP_SCALE + STONE_WARP_OFF_X),
                                        (FNLfloat)(WZ * STONE_WARP_SCALE + STONE_WARP_OFF_X));
    const double WARP_Y = fnlGetNoise3D(&fnl_StoneWarp,
                                        (FNLfloat)(WX * STONE_WARP_SCALE + STONE_WARP_OFF_Y),
                                        (FNLfloat)(WY * STONE_WARP_SCALE + STONE_WARP_OFF_Y),
                                        (FNLfloat)(WZ * STONE_WARP_SCALE + STONE_WARP_OFF_Y));
    const double WARP_Z = fnlGetNoise3D(&fnl_StoneWarp,
                                        (FNLfloat)(WX * STONE_WARP_SCALE + STONE_WARP_OFF_Z),
                                        (FNLfloat)(WY * STONE_WARP_SCALE + STONE_WARP_OFF_Z),
                                        (FNLfloat)(WZ * STONE_WARP_SCALE + STONE_WARP_OFF_Z));

    const double NOISE_X = WX * STONE_SCALE + WARP_X * STONE_WARP_AMPLITUDE;
    const double NOISE_Y = WY * STONE_SCALE + WARP_Y * STONE_WARP_AMPLITUDE;
    const double NOISE_Z = WZ * STONE_SCALE + WARP_Z * STONE_WARP_AMPLITUDE;

    return (float)fnlGetNoise3D(&fnl_Stone, (FNLfloat)NOISE_X, (FNLfloat)NOISE_Y, (FNLfloat)NOISE_Z);
}

static void randomNoise_stone_init(const uint32_t WORLD_SEED)
{
    fnl_Stone = fnlCreateState();
    fnl_Stone.seed = (int)WORLD_SEED;
    fnl_Stone.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_Stone.frequency = 1.0;
    fnl_Stone.fractal_type = FNL_FRACTAL_FBM;
    fnl_Stone.octaves = 5;
    fnl_Stone.lacunarity = 2.0F;
    fnl_Stone.gain = 0.45F;

    fnl_StoneWarp = fnlCreateState();
    // Decorrelate seed but keeps determinism
    fnl_StoneWarp.seed = (int)(WORLD_SEED ^ 0xA53B1F29U);
    fnl_StoneWarp.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_StoneWarp.frequency = 1.0;
    fnl_StoneWarp.fractal_type = FNL_FRACTAL_FBM;
    fnl_StoneWarp.octaves = 4;
    fnl_StoneWarp.lacunarity = 2.1F;
    fnl_StoneWarp.gain = 0.15F;
}
#pragma endregion
#pragma region Carving
static fnl_state fnl_WormField;
static fnl_state fnl_WormWarp;
static fnl_state fnl_RavineHugePath2D;
static fnl_state fnl_RavineHugeWarp2D;
static fnl_state fnl_RavineYCenter;
static fnl_state fnl_GlobalFeatureDensityXZ;
static fnl_state fnl_GlobalFeatureDensity3D;
// Low-frequency scales (larger scale => slower variation => big regions with/without features)
// very low freq in XZ (hundreds to thousands of blocks)
static const double GLOBAL_FEATURE_DENSITY_SCL_XZ = 0.030;
// very low freq in 3D
static const double GLOBAL_FEATURE_DENSITY_SCL_3D = 0.036;
// Per-feature keep fractions (1.0 keeps all occurrences; smaller => sparser)
static const float GLOBAL_FEATURE_KEEP_WORMS = 0.35F;
static const float GLOBAL_FEATURE_KEEP_RAVINES = 0.52F;
// Gate edge softness small => crisper on/off
static const float GLOBAL_FEATURE_GATE_SOFTNESS = 0.24F;

static void randomNoise_carving_init(const uint32_t WORLD_SEED)
{
    // Worm field
    fnl_WormField = fnlCreateState();
    fnl_WormField.seed = (int)(WORLD_SEED ^ 0x5EEDC0DEU);
    fnl_WormField.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_WormField.frequency = 1.0;
    fnl_WormField.fractal_type = FNL_FRACTAL_FBM;
    fnl_WormField.octaves = 4;
    fnl_WormField.lacunarity = 2.03F;
    fnl_WormField.gain = 0.52F;

    fnl_WormWarp = fnlCreateState();
    fnl_WormWarp.seed = (int)(WORLD_SEED ^ 0xBADC0FFEU);
    fnl_WormWarp.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_WormWarp.frequency = 1.0;
    fnl_WormWarp.fractal_type = FNL_FRACTAL_FBM;
    fnl_WormWarp.octaves = 3;
    fnl_WormWarp.lacunarity = 2.11F;
    fnl_WormWarp.gain = 0.45F;

    // Ravine huge path (2D + warp)
    fnl_RavineHugePath2D = fnlCreateState();
    fnl_RavineHugePath2D.seed = (int)(WORLD_SEED ^ 0x3A589BDEU);
    fnl_RavineHugePath2D.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_RavineHugePath2D.frequency = 1.0;
    fnl_RavineHugePath2D.fractal_type = FNL_FRACTAL_RIDGED;
    fnl_RavineHugePath2D.octaves = 3;
    fnl_RavineHugePath2D.lacunarity = 2.0f;
    fnl_RavineHugePath2D.gain = 0.5F;

    fnl_RavineHugeWarp2D = fnlCreateState();
    fnl_RavineHugeWarp2D.seed = (int)(WORLD_SEED ^ 0xA0E8ACE0U);
    fnl_RavineHugeWarp2D.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_RavineHugeWarp2D.frequency = 1.0;
    fnl_RavineHugeWarp2D.fractal_type = FNL_FRACTAL_FBM;
    fnl_RavineHugeWarp2D.octaves = 5;
    fnl_RavineHugeWarp2D.lacunarity = 1.15F;
    fnl_RavineHugeWarp2D.gain = 0.2F;

    // Feature density XZ (low frequency)
    fnl_GlobalFeatureDensityXZ = fnlCreateState();
    fnl_GlobalFeatureDensityXZ.seed = (int)(WORLD_SEED ^ 0xA1F0D3B5U);
    fnl_GlobalFeatureDensityXZ.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_GlobalFeatureDensityXZ.frequency = 1.0;
    fnl_GlobalFeatureDensityXZ.fractal_type = FNL_FRACTAL_FBM;
    fnl_GlobalFeatureDensityXZ.octaves = 3;
    fnl_GlobalFeatureDensityXZ.lacunarity = 2.03F;
    fnl_GlobalFeatureDensityXZ.gain = 0.45F;

    // Feature density 3D (worms benefit from volumetric gating)
    fnl_GlobalFeatureDensity3D = fnlCreateState();
    fnl_GlobalFeatureDensity3D.seed = (int)(WORLD_SEED ^ 0xC7E9B261U);
    fnl_GlobalFeatureDensity3D.noise_type = FNL_NOISE_OPENSIMPLEX2;
    fnl_GlobalFeatureDensity3D.frequency = 1.0;
    fnl_GlobalFeatureDensity3D.fractal_type = FNL_FRACTAL_FBM;
    fnl_GlobalFeatureDensity3D.octaves = 3;
    fnl_GlobalFeatureDensity3D.lacunarity = 2.01F;
    fnl_GlobalFeatureDensity3D.gain = 0.47F;
}
#pragma region Worms
// coarser -> wider curves
static const double WORM_FIELD_SCALE = 0.0075;
// domain warp frequency
static const double WORM_WARP_SCALE = 0.02;
static const double WORM_WARP_AMPL = 12.0;
static const double WORM_RADIUS = 0.10;
static const double WORM_FALLOFF = 0.10;
static const double WORM_WARP_OFF_X = 73.19;
static const double WORM_WARP_OFF_Y = 19.47;
static const double WORM_WARP_OFF_Z = 46.73;
/// @brief Perlin-worm tunnels. Carves slender, windy tubes. [0, 1]
float randomNoise_carve_stageWorms(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3f_t ORIGIN = cmath_chunk_blockPosPacked_2_worldSamplePos(CHUNK_POS, BLOCK_POS_PACKED12);
    const double WX = (double)ORIGIN.x;
    const double WY = (double)ORIGIN.y;
    const double WZ = (double)ORIGIN.z;

    // Domain warp to bend the implicit iso-surface field
    const double WXW = WX + fnlGetNoise3D(&fnl_WormWarp,
                                          (FNLfloat)(WX * WORM_WARP_SCALE + WORM_WARP_OFF_X),
                                          (FNLfloat)(WY * WORM_WARP_SCALE + WORM_WARP_OFF_X),
                                          (FNLfloat)(WZ * WORM_WARP_SCALE + WORM_WARP_OFF_X)) *
                                WORM_WARP_AMPL;
    const double WYW = WY + fnlGetNoise3D(&fnl_WormWarp,
                                          (FNLfloat)(WX * WORM_WARP_SCALE + WORM_WARP_OFF_Y),
                                          (FNLfloat)(WY * WORM_WARP_SCALE + WORM_WARP_OFF_Y),
                                          (FNLfloat)(WZ * WORM_WARP_SCALE + WORM_WARP_OFF_Y)) *
                                WORM_WARP_AMPL;
    const double WZW = WZ + fnlGetNoise3D(&fnl_WormWarp,
                                          (FNLfloat)(WX * WORM_WARP_SCALE + WORM_WARP_OFF_Z),
                                          (FNLfloat)(WY * WORM_WARP_SCALE + WORM_WARP_OFF_Z),
                                          (FNLfloat)(WZ * WORM_WARP_SCALE + WORM_WARP_OFF_Z)) *
                                WORM_WARP_AMPL;

    const double V = (double)fnlGetNoise3D(&fnl_WormField,
                                           (FNLfloat)(WXW * WORM_FIELD_SCALE),
                                           (FNLfloat)(WYW * WORM_FIELD_SCALE),
                                           (FNLfloat)(WZW * WORM_FIELD_SCALE));

    const float D = (float)fabs(V);
    const float MASK = cmath_noise_smoothInvBand(D, (float)WORM_RADIUS, (float)WORM_FALLOFF);

    // 3D density gate (reduces frequency, keeps tube thickness unchanged)
    const double DENSITY_3D = fnlGetNoise3D(&fnl_GlobalFeatureDensity3D,
                                            (FNLfloat)(WX * GLOBAL_FEATURE_DENSITY_SCL_3D),
                                            (FNLfloat)(WY * GLOBAL_FEATURE_DENSITY_SCL_3D),
                                            (FNLfloat)(WZ * GLOBAL_FEATURE_DENSITY_SCL_3D));
    const float GATE_3D = cmath_noise_densityKeepGate(DENSITY_3D, GLOBAL_FEATURE_KEEP_WORMS, GLOBAL_FEATURE_GATE_SOFTNESS);

    return cmath_clampF01(MASK * GATE_3D);
}
#pragma endregion
#pragma region Ravines
// large-scale meanders
static const double RAVINE_HUGE_PATH_SCALE = 0.0009;
static const double RAVINE_HUGE_WARP_SCALE = 0.00035;
static const double RAVINE_HUGE_WARP_AMPL = 5.0;
// center band (|val| small)
static const double RAVINE_HUGE_HALF_WIDTH = 0.058;
// soft edge
static const double RAVINE_HUGE_FALLOFF = 0.08;
// vertical modulation freq
static const double RAVINE_HUGE_Y_SHAPE_SCL = 0.016;
// bias for mid/low altitude
static const double RAVINE_HUGE_Y_SHAPE_AM = 0.035;
static const double RAVINE_HUGE_WARP_OFF_X = 33.33;
static const double RAVINE_HUGE_WARP_OFF_Z = 55.55;
// center height for ravines (blocks)
static const double RAVINE_HUGE_Y_CENTER = -24.0;
static const double RAVINE_HUGE_HALF_HEIGHT = 11.0;
// feather on the vertical envelope
static const double RAVINE_HUGE_Y_SOFT = 20.0;
// lower -> slower vertical climate changes
static const double RAVINE_Y_CENTER_SCL = 0.00035;
// how far up/down ravines can migrate (blocks)
static const double RAVINE_Y_DRIFT_AMPL = 300.0;
/// @brief Long chasms (huge) defined by 2D meanders with vertical shaping. [0, 1]
float randomNoise_carve_stageRavinesHuge(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3f_t ORIGIN = cmath_chunk_blockPosPacked_2_worldSamplePos(CHUNK_POS, BLOCK_POS_PACKED12);
    const double WX = (double)ORIGIN.x;
    const double WY = (double)ORIGIN.y;
    const double WZ = (double)ORIGIN.z;

    const double WDX = fnlGetNoise2D(&fnl_RavineHugePath2D,
                                     (FNLfloat)(WX * RAVINE_HUGE_WARP_SCALE + RAVINE_HUGE_WARP_OFF_X),
                                     (FNLfloat)(WZ * RAVINE_HUGE_WARP_SCALE + RAVINE_HUGE_WARP_OFF_Z)) *
                       RAVINE_HUGE_WARP_AMPL;
    const double WDZ = fnlGetNoise2D(&fnl_RavineHugeWarp2D,
                                     (FNLfloat)(WX * RAVINE_HUGE_WARP_SCALE + RAVINE_HUGE_WARP_OFF_Z),
                                     (FNLfloat)(WZ * RAVINE_HUGE_WARP_SCALE + RAVINE_HUGE_WARP_OFF_X)) *
                       RAVINE_HUGE_WARP_AMPL;

    const double PX = WX + WDX;
    const double PZ = WZ + WDZ;

    const double PATH = (double)fnlGetNoise2D(&fnl_RavineHugePath2D,
                                              (FNLfloat)(PX * RAVINE_HUGE_PATH_SCALE),
                                              (FNLfloat)(PZ * RAVINE_HUGE_PATH_SCALE));

    // ultra-thin horizontal band around |PATH| ~ 0
    float base = cmath_noise_smoothInvBand((float)fabs(PATH),
                                           (float)RAVINE_HUGE_HALF_WIDTH,
                                           (float)RAVINE_HUGE_FALLOFF);

    // sharpen the thinness aggressively (base^4)
    base = base * base;
    base = base * base;

    // Low-frequency vertical “climate” so ravine altitudes wander over long distances
    const double Y_FIELD = fnlGetNoise3D(&fnl_RavineYCenter,
                                         (FNLfloat)(WX * RAVINE_Y_CENTER_SCL),
                                         (FNLfloat)(WY * RAVINE_Y_CENTER_SCL * 0.25), // mild dependence on depth
                                         (FNLfloat)(WZ * RAVINE_Y_CENTER_SCL));
    const double Y_CENTER = Y_FIELD * RAVINE_Y_DRIFT_AMPL;

    // Distance from the local center band
    const double Y_DIST = fabs(WY - Y_CENTER);
    const float VENV = cmath_noise_smoothInvBand((float)Y_DIST,
                                                 (float)RAVINE_HUGE_HALF_HEIGHT,
                                                 (float)RAVINE_HUGE_Y_SOFT);

    // final mask: thin lateral * tight vertical
    float mask = cmath_clampF01(base * cmath_clampF01(base * VENV));

    // XZ density gate (reduces how often ravines exist; does not change width/height)
    const double DENSITY_2D = fnlGetNoise2D(&fnl_GlobalFeatureDensityXZ,
                                            (FNLfloat)(WX * GLOBAL_FEATURE_DENSITY_SCL_XZ),
                                            (FNLfloat)(WZ * GLOBAL_FEATURE_DENSITY_SCL_XZ));
    const float GATE_2D = cmath_noise_densityKeepGate(DENSITY_2D, GLOBAL_FEATURE_KEEP_RAVINES, GLOBAL_FEATURE_GATE_SOFTNESS);

    return cmath_clampF01(mask * GATE_2D);
}
#pragma endregion
#pragma region Carving Sample
float randomNoise_carving_sampleXYZ(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const float WORMS = randomNoise_carve_stageWorms(CHUNK_POS, BLOCK_POS_PACKED12);
    const float RAVINES_HUGE = randomNoise_carve_stageRavinesHuge(CHUNK_POS, BLOCK_POS_PACKED12);

    const float ONE_MINUS_CARVE =
        (1.0F - cmath_clampF01(WORMS)) *
        (1.0F - cmath_clampF01(RAVINES_HUGE));

    return ONE_MINUS_CARVE;
}
#pragma endregion
#pragma endregion
#pragma region Init
/// @brief Initialize the noise system.
void randomNoise_init(const uint32_t WORLD_SEED)
{
    randomNoise_carving_init(WORLD_SEED);
    randomNoise_stone_init(WORLD_SEED);
}
#pragma endregion