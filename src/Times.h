#pragma once

#include "Headers.h"

void timeInit(State_t *state)
{
    Time_t time = {
        .fixedTimeAccumulated = 0.0,
        .frameTimeDelta = 0.0,
        .frameTimeLast = 0.0,
        .frameTimeTotal = 0.0,
    };

    glfwSetTime(0.0);
}

void timeUpdate(State_t *state)
{
    double currentTime = glfwGetTime();
    state->time.frameTimeDelta = currentTime - state->time.frameTimeLast;
    state->time.frameTimeLast = currentTime;
    state->time.fixedTimeAccumulated += state->time.frameTimeDelta;
    state->time.frameTimeTotal += state->time.frameTimeDelta;
    state->time.framesPerSecond = 1.0 / state->time.frameTimeDelta;
}