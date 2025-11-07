#pragma once

struct Character_t;
struct Chunk_t;
typedef struct
{
    // Character that caused the event
    struct Character_t *pCharacterEventSource;
    struct Chunk_t *pChunk;
} CtxChunk_t;