#pragma once

typedef struct
{
    // Time since last frame
    double CPU_frameTimeDelta;
    // Actual last time (not delta)
    double CPU_frameTimeLast;
    double CPU_frameTimeTotal;
    double CPU_framesPerSecond;
    // Fixed-step physics
    double CPU_fixedTimeAccumulated;
} Time_t;