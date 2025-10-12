#pragma once

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