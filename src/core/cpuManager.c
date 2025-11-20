#pragma region Includes
#include <stdint.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#pragma endregion
#pragma region Defines
// #define NARRATE_CPU_TIME
// This ** MUST ** be a power of 2!
#define CPU_TIME_SAMPLES 128

static bool rollingDeltaCalcThisFrame = false;
static uint16_t measurementIndex = 0;
static uint16_t sampleCount = 0;
static double pCPUTimeMeasurements[CPU_TIME_SAMPLES];
static double cpuTimeMeasurementLast = 0.0;
static double rollingDelta = 0.0;
static double rollingSum = 0.0;

#pragma endregion
#pragma region Operations
double cpuManager_rollingDeltaTimeAvg(void)
{
    if (sampleCount == 0)
        return 0.0;

    if (!rollingDeltaCalcThisFrame)
    {
        rollingDelta = rollingSum / (double)sampleCount;
        rollingDeltaCalcThisFrame = true;
    }

    return rollingDelta;
}

double cpuManager_lastDeltaTime(void)
{
    return cpuTimeMeasurementLast;
}

void cpuManager_captureDeltaTime(State_t *pState)
{
    double newSample = pState->time.CPU_frameTimeDelta;
    double oldSample = pCPUTimeMeasurements[measurementIndex];

    rollingSum -= oldSample;
    rollingSum += newSample;

    pCPUTimeMeasurements[measurementIndex] = newSample;
    cpuTimeMeasurementLast = newSample;

    // Power-of-two wrap
    measurementIndex = (measurementIndex + 1) & (CPU_TIME_SAMPLES - 1);

    if (sampleCount < CPU_TIME_SAMPLES)
        ++sampleCount;

    rollingDeltaCalcThisFrame = false;

#if defined(NARRATE_CPU_TIME)
    if (measurementIndex % CPU_TIME_SAMPLES == 0)
        logs_log(LOG_DEBUG, "CPU Frame Time Average = %lf (CPU 'FPS' %lf) Last = %lf (CPU 'FPS' %lf)",
                 cpuManager_rollingDeltaTimeAvg(), 1.0 / cpuManager_rollingDeltaTimeAvg(),
                 cpuManager_lastDeltaTime(), 1.0 / cpuManager_lastDeltaTime());
#endif
}
#pragma endregion