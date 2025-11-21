#pragma once

typedef enum WorldType_e
{
    WORLD_TYPE_LOCAL,
    WORLD_TYPE_SERVER,
} WorldType_e;

static const char *pWORLD_TYPE_NAMES[] = {
    "WORLD_TYPE_LOCAL",
    "WORLD_TYPE_SERVER"};