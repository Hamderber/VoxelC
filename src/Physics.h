#pragma once

void physicsUpdate(State_t *state)
{
    // logger(LOG_PHYSICS, "Physics loop. Fixed delta time = %lf", state->time.fixedTimeAccumulated);
}

void physicsLoop(State_t *state)
{
    double numPhysicsFrames = state->time.fixedTimeAccumulated / state->config.fixedTimeStep;

    if (numPhysicsFrames > state->config.maxPhysicsFrameDelay)
    {
        // This is on the main thread, which means that it gets locked while the actual window resizing is taking place
        logger(LOG_PHYSICS, "Physics is running %.lf frames behind! Skipping %.lf frames. If the window was just resized, this is expected behaviour.",
               numPhysicsFrames, numPhysicsFrames - state->config.maxPhysicsFrameDelay);

        state->time.fixedTimeAccumulated = state->config.fixedTimeStep * state->config.maxPhysicsFrameDelay;
    }

    while (state->time.fixedTimeAccumulated >= state->config.fixedTimeStep)
    {
        physicsUpdate(state);
        state->time.fixedTimeAccumulated -= state->config.fixedTimeStep;
    }
}
