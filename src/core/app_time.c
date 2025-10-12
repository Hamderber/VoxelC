#include <GLFW/glfw3.h>
#include "core/state/state.h"
#include "app_time.h"

/// @brief Sets app (GLFW) time and state time to zero
/// @param state
void time_init(Time_t *time)
{
    *time = (Time_t){
        .fixedTimeAccumulated = 0.0,
        .frameTimeDelta = 0.0,
        .frameTimeLast = 0.0,
        .frameTimeTotal = 0.0,
    };

    glfwSetTime(0.0);
}

/// @brief Updates state time (including deltas) with current GLFW time
/// @param state
void time_update(Time_t *time)
{
    double currentTime = glfwGetTime();
    time->frameTimeDelta = currentTime - time->frameTimeLast;
    time->frameTimeLast = currentTime;
    time->fixedTimeAccumulated += time->frameTimeDelta;
    time->frameTimeTotal += time->frameTimeDelta;
    time->framesPerSecond = 1.0 / time->frameTimeDelta;
}
