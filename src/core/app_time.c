#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "app_time.h"

/// @brief Sets app (GLFW) time and state time to zero
/// @param state
void time_init(Time_t *pTime)
{
    *pTime = (Time_t){
        .CPU_fixedTimeAccumulated = 0.0,
        .CPU_frameTimeDelta = 0.0,
        .CPU_frameTimeLast = 0.0,
        .CPU_frameTimeTotal = 0.0,
    };

    glfwSetTime(0.0);
}

/// @brief Updates state time (including deltas) with current GLFW time
/// @param state
void time_update(Time_t *pTime)
{
    double currentTime = glfwGetTime();
    pTime->CPU_frameTimeDelta = currentTime - pTime->CPU_frameTimeLast;
    pTime->CPU_frameTimeLast = currentTime;
    pTime->CPU_fixedTimeAccumulated += pTime->CPU_frameTimeDelta;
    pTime->CPU_frameTimeTotal += pTime->CPU_frameTimeDelta;
    pTime->CPU_framesPerSecond = 1.0 / pTime->CPU_frameTimeDelta;
}
