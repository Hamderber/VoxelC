#pragma once

#include <stdint.h>

void randomNoise_init(const uint32_t WORLD_SEED);

float randomNoise_stone_samplePackedPos(const Chunk_t *pC, const uint16_t BLOCK_POS_PACKED12);

float randomNoise_carving_sampleXYZ(const Chunk_t *pC, const uint16_t BLOCK_POS_PACKED12);
