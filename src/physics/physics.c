#include "core/logs.h"
#include "physics.h"
#include "core/random.h"
#include "cmath/cmath.h"
#include "character/characterController.h"
#include "scene/scene.h"

void phys_applyForce(EntityDataPhysics_t *p, Vec3f_t force, float mass)
{
    // f=ma
    Vec3f_t a = cmath_vec3f_mult_scalarF(force, 1.0F / mass);
    p->transientAcceleration = cmath_vec3f_add_vec3f(p->transientAcceleration, a);
}

void phys_applyImpulse(EntityDataPhysics_t *p, Vec3f_t impulse)
{
    // impulse in m/s add directly to velocity
    p->velocity = cmath_vec3f_add_vec3f(p->velocity, impulse);
}

// Uses the physics data's uniformSpeed
void phys_applyImpulseUniform(EntityDataPhysics_t *p, Vec3f_t direction, bool isNormalized)
{
    if (isNormalized)
        p->velocity = cmath_vec3f_add_vec3f(p->velocity,
                                            cmath_vec3f_mult_scalarF(direction, p->uniformSpeed));
    else
        p->velocity = cmath_vec3f_add_vec3f(p->velocity,
                                            cmath_vec3f_mult_scalarF(cmath_vec3f_normalize(direction), p->uniformSpeed));
}

void phys_applyRotationIntention(EntityDataPhysics_t *p, float dt)
{
    // Convert angular velocity (rad/s) -> quaternion delta
    if (!cmath_vec3f_isZero(p->rotationIntentionEulerRad))
    {
        Vec3f_t delta = cmath_vec3f_mult_scalarF(p->rotationIntentionEulerRad, dt);
        float angle = cmath_vec3f_magnitudeF(delta);

        if (angle > CMATH_EPSILON_F)
        {
            Vec3f_t axis = cmath_vec3f_mult_scalarF(delta, 1.0f / angle);
            Quaternionf_t dq = cmath_quat_fromAxisAngle(angle, axis);

            if (p->useLocalAxes)
                // local rotation
                p->rotation = cmath_quat_normalize(cmath_quat_mult_quat(p->rotation, dq));
            else
                // world rotation
                p->rotation = cmath_quat_normalize(cmath_quat_mult_quat(dq, p->rotation));
        }

        p->rotationIntentionEulerRad = VEC3F_ZERO;
    }

    // Per-frame quaternion delta (already time-scaled)
    if (!cmath_quat_isIdentity(p->rotationIntentionQuat))
    {
        if (p->useLocalAxes)
            p->rotation = cmath_quat_normalize(cmath_quat_mult_quat(p->rotation, p->rotationIntentionQuat));
        else
            p->rotation = cmath_quat_normalize(cmath_quat_mult_quat(p->rotationIntentionQuat, p->rotation));

        p->rotationIntentionQuat = QUATERNION_IDENTITY;
    }
}

void phys_applyMoveIntention(EntityDataPhysics_t *p, float dt)
{
    Vec3f_t dir = p->moveIntention;
    if (p->useLocalAxes)
        dir = cmath_quat_rotateVec3(p->rotation, dir);

    dir = cmath_vec3f_normalize(dir);

    // Scale by dt because uniformSpeed is in m/s and we're applying every tick
    Vec3f_t scaledDir = cmath_vec3f_mult_scalarF(dir, dt);

    static const bool IS_NORMALIZED = true;
    phys_applyImpulseUniform(p, scaledDir, IS_NORMALIZED);
}

void phys_integrate(EntityDataPhysics_t *p, float dt)
{
    p->velocity = cmath_vec3f_add_vec3f(p->velocity, cmath_vec3f_mult_scalarF(p->transientAcceleration, dt));

    // Apply drag. Don't let a huge drag result in going the opposite direction. Normally drag would be
    // Just 1F - drag * dt but bigger numbers help for things like snappy flight controls etc
    p->velocity = cmath_vec3f_mult_scalarF(p->velocity, cmath_clampF(1.0F - p->drag * dt, 0.0F, CMATH_MAX_F));

    p->worldPosOld = p->worldPos;
    p->worldPos = cmath_vec3f_add_vec3f(p->worldPos, cmath_vec3f_mult_scalarF(p->velocity, dt));

    p->transientAcceleration = VEC3F_ZERO;
}

void phys_entityPhysicsApply(State_t *pState, float dt)
{
    Entity_t **ppEntities = pState->entityManager.entityCollections[ENTITY_COLLECTION_PHYSICS].entities;

    EntityComponentData_t *pComponentData;
    for (size_t i = 0; i < ENTITIES_MAX_IN_COLLECTION; i++)
    {
        if (ppEntities[i] == NULL || ppEntities[i]->componentCount == 0 || ppEntities[i]->pComponents == NULL)
            continue;

        if (em_entityDataGet(ppEntities[i], ENTITY_COMPONENT_TYPE_PHYSICS, &pComponentData))
        {
            phys_applyRotationIntention(pComponentData->pPhysicsData, dt);
            phys_applyMoveIntention(pComponentData->pPhysicsData, dt);
            phys_integrate(pComponentData->pPhysicsData, dt);

            static const float TOLERANCE = CMATH_EPSILON_F;
            if (!cmath_vec3f_equals(pComponentData->pPhysicsData->worldPos, pComponentData->pPhysicsData->worldPosOld, TOLERANCE))
                // Don't bother trying to update chunk position if it didn't change
                entity_chunkPos_update(pState, ppEntities[i], pComponentData);
        }

        pComponentData = NULL;
    }
}

void phys_modelsPhysicsApply(State_t *pState, float dt)
{
    float rad = PI_F / 4.0F * dt;
    scene_debug_rotateAllRandom(&pState->scene, rad);
}

void phys_update(State_t *pState)
{
    float dt = (float)pState->config.fixedTimeStep;

    // Directly update player's movement intent
    player_physicsIntentUpdate(pState);

    phys_entityPhysicsApply(pState, dt);
    phys_modelsPhysicsApply(pState, dt);
}

void phys_loop(State_t *pState)
{
    // If the world isn't loaded, don't do physics frames.
    if (!pState || !pState->pWorldState || !pState->pWorldState->isLoaded)
        return;

    double numPhysicsFrames = pState->time.fixedTimeAccumulated / pState->config.fixedTimeStep;

    if (numPhysicsFrames > pState->config.maxPhysicsFrameDelay)
    {
        // This is on the main thread, which means that it gets locked while the actual window resizing is taking place
        logs_log(LOG_PHYSICS, "Physics is running %.lf frames behind! Skipping %.lf frames. If the window was just resized or moved, \
this is expected behaviour.",
                 numPhysicsFrames, numPhysicsFrames - pState->config.maxPhysicsFrameDelay);

        pState->time.fixedTimeAccumulated = pState->config.fixedTimeStep * pState->config.maxPhysicsFrameDelay;
    }

    while (pState->time.fixedTimeAccumulated >= pState->config.fixedTimeStep)
    {
        phys_update(pState);
        pState->time.fixedTimeAccumulated -= pState->config.fixedTimeStep;
    }
}