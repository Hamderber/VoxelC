#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "app_time.h"

/// @brief Sets app (GLFW) time and state time to zero
/// @param state
void time_init(Time_t *pTime)
{
    *pTime = (Time_t){
        .fixedTimeAccumulated = 0.0,
        .frameTimeDelta = 0.0,
        .frameTimeLast = 0.0,
        .frameTimeTotal = 0.0,
    };

    glfwSetTime(0.0);
}

/// @brief Updates state time (including deltas) with current GLFW time
/// @param state
void time_update(Time_t *pTime)
{
    double currentTime = glfwGetTime();
    pTime->frameTimeDelta = currentTime - pTime->frameTimeLast;
    pTime->frameTimeLast = currentTime;
    pTime->fixedTimeAccumulated += pTime->frameTimeDelta;
    pTime->frameTimeTotal += pTime->frameTimeDelta;
    pTime->framesPerSecond = 1.0 / pTime->frameTimeDelta;
}
