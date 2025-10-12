#include <GLFW/glfw3.h>
#include "core/state.h"
#include "app_time.h"

/// @brief Sets app (GLFW) time and state time to zero
/// @param state
void time_init(State_t *state)
{
    Time_t time = {
        .fixedTimeAccumulated = 0.0,
        .frameTimeDelta = 0.0,
        .frameTimeLast = 0.0,
        .frameTimeTotal = 0.0,
    };

    glfwSetTime(0.0);
}

/// @brief Updates state time (including deltas) with current GLFW time
/// @param state
void time_update(State_t *state)
{
    double currentTime = glfwGetTime();
    state->time.frameTimeDelta = currentTime - state->time.frameTimeLast;
    state->time.frameTimeLast = currentTime;
    state->time.fixedTimeAccumulated += state->time.frameTimeDelta;
    state->time.frameTimeTotal += state->time.frameTimeDelta;
    state->time.framesPerSecond = 1.0 / state->time.frameTimeDelta;
}
