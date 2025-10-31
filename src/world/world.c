#include <string.h>
#include <stdlib.h>
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/buffers/index_buffer.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "world/world_t.h"
#include "character/characterType_t.h"
#include "character/character.h"
#include "rendering/types/shaderVertex_t.h"
#include "rendering/chunk/chunkRendering.h"
#include "core/types/atlasRegion_t.h"
#include "core/types/atlasFace_t.h"
#include "rendering/voxel.h"
#include "rendering/types/faceTexture_t.h"
#include "rendering/uvs.h"
#include "chunk.h"
#include "rendering/chunk/chunkRendering.h"

static RenderChunk_t *world_dummyChunkCreate(State_t *state, Vec3f_t position)
{
    const int vertsPerFace = 4;
    const int indicesPerFace = 6;
    const int faceCount = 6;
    const size_t vertexCount = (size_t)faceCount * (size_t)vertsPerFace;  // 24
    const size_t indexCount = (size_t)faceCount * (size_t)indicesPerFace; // 36

    const FaceTexture_t FACE_TEXTURES[6] = {
        [FACE_LEFT] = {OBSIDIAN, TEX_ROT_0},
        [FACE_RIGHT] = {OBSIDIAN, TEX_ROT_0},
        [FACE_TOP] = {OBSIDIAN, TEX_ROT_0},
        [FACE_BOTTOM] = {OBSIDIAN, TEX_ROT_0},
        [FACE_FRONT] = {OBSIDIAN, TEX_ROT_0},
        [FACE_BACK] = {OBSIDIAN, TEX_ROT_0},
    };

    // ORDER REQUIRED: [0]=TL, [1]=BL, [2]=TR, [3]=BR (viewed from OUTSIDE), CCW
    const Vec3f_t FACE_POSITIONS[6][4] = {
        // LEFT  (-X), view from -X → +X
        [FACE_LEFT] = {
            VEC3_VOXEL_BACK_TOP_LEFT,  // TL (0,1,0)
            VEC3_VOXEL_BACK_BOT_LEFT,  // BL (0,0,0)
            VEC3_VOXEL_FRONT_TOP_LEFT, // TR (0,1,1)
            VEC3_VOXEL_FRONT_BOT_LEFT  // BR (0,0,1)
        },
        // RIGHT (+X), view from +X → -X
        [FACE_RIGHT] = {
            VEC3_VOXEL_FRONT_TOP_RIGHT, // TL (1,1,1)
            VEC3_VOXEL_FRONT_BOT_RIGHT, // BL (1,0,1)
            VEC3_VOXEL_BACK_TOP_RIGHT,  // TR (1,1,0)
            VEC3_VOXEL_BACK_BOT_RIGHT   // BR (1,0,0)
        },
        // TOP (+Y), view from +Y (down), screen-up = -Z, screen-right = +X
        [FACE_TOP] = {
            VEC3_VOXEL_BACK_TOP_LEFT,  // TL (0,1,0)
            VEC3_VOXEL_FRONT_TOP_LEFT, // BL (0,1,1)
            VEC3_VOXEL_BACK_TOP_RIGHT, // TR (1,1,0)
            VEC3_VOXEL_FRONT_TOP_RIGHT // BR (1,1,1)
        },
        // BOTTOM (-Y), view from -Y (up), screen-up = +Z, screen-right = +X
        [FACE_BOTTOM] = {
            VEC3_VOXEL_FRONT_BOT_LEFT,  // TL (0,0,1)
            VEC3_VOXEL_BACK_BOT_LEFT,   // BL (0,0,0)
            VEC3_VOXEL_FRONT_BOT_RIGHT, // TR (1,0,1)
            VEC3_VOXEL_BACK_BOT_RIGHT   // BR (1,0,0)
        },
        // FRONT (+Z), view from +Z → -Z
        [FACE_FRONT] = {
            VEC3_VOXEL_FRONT_TOP_LEFT,  // TL (0,1,1)
            VEC3_VOXEL_FRONT_BOT_LEFT,  // BL (0,0,1)
            VEC3_VOXEL_FRONT_TOP_RIGHT, // TR (1,1,1)
            VEC3_VOXEL_FRONT_BOT_RIGHT  // BR (1,0,1)
        },
        // BACK  (-Z), view from -Z → +Z
        [FACE_BACK] = {
            VEC3_VOXEL_BACK_TOP_RIGHT, // TL (1,1,0)
            VEC3_VOXEL_BACK_BOT_RIGHT, // BL (1,0,0)
            VEC3_VOXEL_BACK_TOP_LEFT,  // TR (0,1,0)
            VEC3_VOXEL_BACK_BOT_LEFT   // BR (0,0,0)
        },
    };

    ShaderVertex_t *vertices = (ShaderVertex_t *)calloc(vertexCount, sizeof(ShaderVertex_t));
    uint16_t *indices = (uint16_t *)calloc(indexCount, sizeof(uint16_t));
    if (!vertices || !indices)
    {
        free(vertices);
        free(indices);
        return NULL;
    }

    // Fill pos/color/atlasIndex
    for (int face = 0; face < faceCount; ++face)
    {
        const FaceTexture_t tex = FACE_TEXTURES[face];
        const int base = face * vertsPerFace;

        vertices[base + 0].pos = FACE_POSITIONS[face][0]; // TL
        vertices[base + 1].pos = FACE_POSITIONS[face][1]; // BL
        vertices[base + 2].pos = FACE_POSITIONS[face][2]; // TR
        vertices[base + 3].pos = FACE_POSITIONS[face][3]; // BR

        vertices[base + 0].color = COLOR_WHITE;
        vertices[base + 1].color = COLOR_WHITE;
        vertices[base + 2].color = COLOR_WHITE;
        vertices[base + 3].color = COLOR_WHITE;

        vertices[base + 0].atlasIndex = tex.atlasIndex;
        vertices[base + 1].atlasIndex = tex.atlasIndex;
        vertices[base + 2].atlasIndex = tex.atlasIndex;
        vertices[base + 3].atlasIndex = tex.atlasIndex;
    }

    // UVs via your proven helpers (faceUVs + rotation → atlas region)
    for (int face = 0; face < faceCount; ++face)
    {
        const FaceTexture_t tex = FACE_TEXTURES[face];
        const AtlasRegion_t *region = &state->renderer.pAtlasRegions[tex.atlasIndex];
        assignFaceUVs(vertices, (size_t)face * (size_t)vertsPerFace, region, tex.rotation);
    }

    // Indices per face (TL, BL, TR, BR) → triangles: {0,1,2} and {2,3,0} (CCW)
    for (int face = 0; face < faceCount; ++face)
    {
        const uint16_t v = (uint16_t)(face * vertsPerFace);
        const size_t i = (size_t)face * (size_t)indicesPerFace;

        // v = face*4; i = face*6
        indices[i + 0] = v + 0; // TL
        indices[i + 1] = v + 1; // BL
        indices[i + 2] = v + 3; // BR
        indices[i + 3] = v + 0; // TL
        indices[i + 4] = v + 3; // BR
        indices[i + 5] = v + 2; // TR
    }

    vertexBuffer_createFromData(state, vertices, (uint32_t)vertexCount);
    indexBuffer_createFromData(state, indices, (uint32_t)indexCount);

    RenderChunk_t *chunk = (RenderChunk_t *)calloc(1, sizeof(RenderChunk_t));
    if (!chunk)
    {
        free(vertices);
        free(indices);
        return NULL;
    }

    chunk->vertexBuffer = state->renderer.vertexBuffer;
    chunk->vertexMemory = state->renderer.vertexBufferMemory;
    chunk->indexBuffer = state->renderer.indexBuffer;
    chunk->indexMemory = state->renderer.indexBufferMemory;
    chunk->indexCount = (uint32_t)indexCount;

    free(vertices);
    free(indices);

    chunk_placeRenderInWorld(chunk, &position);
    return chunk;
}

void world_init(State_t *state)
{
    state->worldState = calloc(1, sizeof(WorldState_t));

    state->worldState->world.pPlayer = character_create(state, CHARACTER_TYPE_PLAYER);

    state->worldState->renderChunkCount = 27;
    state->worldState->ppRenderChunks = calloc(state->worldState->renderChunkCount, sizeof(RenderChunk_t *));

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                size_t idx = (size_t)i * 3 * 3 + (size_t)j * 3 + (size_t)k; // 27 slots
                Vec3f_t pos = {(float)i, (float)j, (float)k};
                state->worldState->ppRenderChunks[idx] = world_dummyChunkCreate(state, pos);
                logs_log(LOG_INFO, "Dummy voxel chunk created at (%f, %f, %f).", pos.x, pos.y, pos.z);
            }
        }
    }
}

void world_load(State_t *state)
{
    world_init(state);
}

void world_destroy(State_t *state)
{
    if (!state || !state->worldState || !state->worldState->ppRenderChunks)
        return;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(state->context.device);

    size_t count = state->worldState->renderChunkCount;
    for (size_t i = 0; i < count; ++i)
    {
        RenderChunk_t *chunk = state->worldState->ppRenderChunks[i];
        if (!chunk)
            continue;

        chunk_renderDestroy(state, chunk);
        state->worldState->ppRenderChunks[i] = NULL;
    }

    // Free the container array and reset counters
    free(state->worldState->ppRenderChunks);
    state->worldState->ppRenderChunks = NULL;
    state->worldState->renderChunkCount = 0;
}