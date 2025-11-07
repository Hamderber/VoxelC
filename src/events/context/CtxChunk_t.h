#pragma once

struct Entity_t;
struct Chunk_t;
typedef struct
{
    // Character that caused the event
    struct Entity_t *pEntitySource;
    struct Chunk_t *pChunk;
} CtxChunk_t;