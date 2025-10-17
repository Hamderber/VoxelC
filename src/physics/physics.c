#include "physics.h"
#include "core/random.h"
#include "c_math/c_math.h"
#include "rendering/camera/cameraController.h"

void phys_applyForce(EntityDataPhysics_t *p, Vec3f_t force, float mass)
{
    // f=ma
    Vec3f_t a = cm_vec3fMultScalar(force, 1.0F / mass);
    p->transientAcceleration = cm_vec3fSum(p->transientAcceleration, a);
}

void phys_applyImpulse(EntityDataPhysics_t *p, Vec3f_t impulse)
{
    // impulse in m/s add directly to velocity
    p->velocity = cm_vec3fSum(p->velocity, impulse);
}

// Uses the physics data's uniformSpeed
void phys_applyImpulseUniform(EntityDataPhysics_t *p, Vec3f_t direction, bool isNormalized)
{
    if (isNormalized)
    {
        p->velocity = cm_vec3fSum(p->velocity,
                                  cm_vec3fMultScalar(direction, p->uniformSpeed));
    }
    else
    {
        p->velocity = cm_vec3fSum(p->velocity,
                                  cm_vec3fMultScalar(cm_vec3fNormalize(direction), p->uniformSpeed));
    }
}

void phys_applyMoveIntention(EntityDataPhysics_t *p, float dt)
{
    Vec3f_t dir = cm_vec3fNormalize(p->moveIntention);

    // Scale by dt because uniformSpeed is in m/s and we're applying every tick
    Vec3f_t scaledDir = cm_vec3fMultScalar(dir, dt);

    phys_applyImpulseUniform(p, scaledDir, true);
}

void phys_integrate(EntityDataPhysics_t *p, float dt)
{
    // Update velocity
    p->velocity = cm_vec3fSum(p->velocity, cm_vec3fMultScalar(p->transientAcceleration, dt));

    // Apply drag. Don't let a huge drag result in going the opposite direction. Normally drag would be
    // Just 1F - drag * dt but bigger numbers help for things like snappy flight controls etc
    p->velocity = cm_vec3fMultScalar(p->velocity, cm_clampf(1.0F - p->drag * dt, 0.0F, FLT_MAX));

    // Update position
    p->pos = cm_vec3fSum(p->pos, cm_vec3fMultScalar(p->velocity, dt));

    // Reset acceleration
    p->transientAcceleration = VEC3_ZERO;
}

void phys_entityPhysicsApply(State_t *state, float dt)
{
    Entity_t **entities = state->entityManager.entityCollections[ENTITY_COLLECTION_PHYSICS].entities;

    EntityComponentData_t *componentData;
    for (size_t i = 0; i < ENTITIES_MAX_IN_COLLECTION; i++)
    {
        if (entities[i] == NULL || entities[i]->componentCount == 0 || entities[i]->components == NULL)
            continue;

        if (em_entityDataGet(entities[i], ENTITY_COMPONENT_TYPE_PHYSICS, &componentData))
        {
            phys_applyMoveIntention(componentData->physicsData, dt);
            phys_integrate(componentData->physicsData, dt);
        }

        componentData = NULL;
    }
}

void phys_update(State_t *state)
{
    float dt = (float)state->config.fixedTimeStep;

    // Update the camera here
    camera_physicsIntentUpdate(state);

    phys_entityPhysicsApply(state, dt);
}

void phys_loop(State_t *state)
{
    double numPhysicsFrames = state->time.fixedTimeAccumulated / state->config.fixedTimeStep;

    if (numPhysicsFrames > state->config.maxPhysicsFrameDelay)
    {
        // This is on the main thread, which means that it gets locked while the actual window resizing is taking place
        logs_log(LOG_PHYSICS, "Physics is running %.lf frames behind! Skipping %.lf frames. If the window was just resized or moved, \
this is expected behaviour.",
                 numPhysicsFrames, numPhysicsFrames - state->config.maxPhysicsFrameDelay);

        state->time.fixedTimeAccumulated = state->config.fixedTimeStep * state->config.maxPhysicsFrameDelay;
    }

    while (state->time.fixedTimeAccumulated >= state->config.fixedTimeStep)
    {
        phys_update(state);
        state->time.fixedTimeAccumulated -= state->config.fixedTimeStep;
    }
}