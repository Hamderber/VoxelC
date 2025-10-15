#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/random.h"

void camera_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing camera...");

    float randRange = 10.0F;
    state->camera.pos = rand_vec3f(-randRange, randRange, -randRange, randRange, -randRange, randRange);

    logs_log(LOG_DEBUG, "Camera is at random position (%lf, %lf, %lf)", state->camera.pos.x, state->camera.pos.y, state->camera.pos.z);
}