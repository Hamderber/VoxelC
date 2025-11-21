#pragma once
#include <stdbool.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "physics/physics.h"

static const double ROLLING_DELTA_THRESHOLD = 0.2;

double cpuManager_rollingDeltaTimeAvg(void);

double cpuManager_lastDeltaTime(void);

static inline bool cpuManager_lightenTheLoad(State_t *restrict pState)
{
    if (!pState)
        return false;

    double rollingDelta = cpuManager_rollingDeltaTimeAvg();
    bool overloaded = rollingDelta > ROLLING_DELTA_THRESHOLD;
    // bool overloaded = phys_isRunningBehind(pState) || rollingDelta > ROLLING_DELTA_THRESHOLD;

    if (overloaded)
        logs_log(LOG_WARN, "CPU is overloaded! CPU Frame Time Average = %lf (CPU 'FPS' %lf) Last = %lf (CPU 'FPS' %lf)",
                 rollingDelta, 1.0 / rollingDelta,
                 cpuManager_lastDeltaTime(), 1.0 / cpuManager_lastDeltaTime());

    return overloaded;
}

void cpuManager_captureDeltaTime(State_t *pState);