#pragma once

void physicsUpdate(State_t *state)
{
    logger(LOG_PHYSICS, "Physics loop. Fixed delta time = %lf", state->time.fixedTimeAccumulated);
}

void physicsLoop(State_t *state)
{
    double numPhysicsFrames = state->time.fixedTimeAccumulated / state->config.fixedTimeStep;

    if (numPhysicsFrames > state->config.maxPhysicsFrameDelay)
    {
        logger(LOG_PHYSICS, "Physics is running %d frames behind! Skipping %d frames.",
               numPhysicsFrames, numPhysicsFrames - state->config.maxPhysicsFrameDelay);

        state->time.fixedTimeAccumulated = state->config.fixedTimeStep * state->config.maxPhysicsFrameDelay;
    }

    while (state->time.fixedTimeAccumulated >= state->config.fixedTimeStep)
    {
        physicsUpdate(state);
        state->time.fixedTimeAccumulated -= state->config.fixedTimeStep;
    }
}
