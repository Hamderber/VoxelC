#pragma once

// #include "core/state.h"

typedef struct
{
    // Time since last frame
    double frameTimeDelta;
    // Actual last time (not delta)
    double frameTimeLast;
    double frameTimeTotal;
    double framesPerSecond;
    // Fixed-step physics
    double fixedTimeAccumulated;
} Time_t;

void time_init(Time_t *time);

void time_update(Time_t *time);
