#pragma once

#include "core/types/state_t.h"

static inline double phys_framesBehind_get(State_t *pState) { return pState->time.CPU_fixedTimeAccumulated / pState->config.fixedTimeStep; }

static inline bool phys_isRunningBehind(State_t *pState) { return phys_framesBehind_get(pState) > pState->config.maxPhysicsFrameDelay; }

void phys_update(State_t *state);

void phys_loop(State_t *state);
