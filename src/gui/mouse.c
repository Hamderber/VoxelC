#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "c_math/c_math.h"

static double lastX = 0.0, lastY = 0.0;
static bool firstRecord = true;

void mouse_onMove(GLFWwindow *window, double xpos, double ypos)
{
    // Unused
    window;

    if (firstRecord)
    {
        firstRecord = false;
        lastX = xpos;
        lastY = ypos;
    }

    State_t *state = glfwGetWindowUserPointer(window);

    double dx = xpos - lastX;
    // Y is inverse because the window tracks from top to bottom but dx/dy is left to right and down to up (normal graphing)
    double dy = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    EntityComponentData_t *componentData;
    if (em_entityDataGet(state->context.pCameraEntity,
                         ENTITY_COMPONENT_TYPE_PHYSICS,
                         &componentData))
    {
        EntityDataPhysics_t *phys = componentData->physicsData;

        // Convert mouse delta to radians
        const float radPerPixel = 0.0025f * (float)state->config.mouseSensitivity;
        const float yawDelta = (float)(-dx * radPerPixel);
        const float pitchDelta = (float)(dy * radPerPixel);

        // --- 1) Apply yaw around WORLD up (always global) ---
        Quaternion_t qYaw = cm_quatFromAxisAngle(yawDelta, UP);
        phys->rotation = cm_quatNormalize(cm_quatMultiply(qYaw, phys->rotation));

        // --- 2) Apply pitch around the camera's LOCAL right axis ---
        Vec3f_t right = cm_quatRotateVec3(phys->rotation, RIGHT);
        Quaternion_t qPitch = cm_quatFromAxisAngle(pitchDelta, right);
        phys->rotation = cm_quatNormalize(cm_quatMultiply(phys->rotation, qPitch));

        // --- 3) Clamp pitch ---
        Vec3f_t euler = cm_quatToEulerAngles(phys->rotation);
        const float maxPitch = (PI_F * 0.5f) - 0.001f;
        if (euler.x > maxPitch)
            euler.x = maxPitch;
        if (euler.x < -maxPitch)
            euler.x = -maxPitch;
        euler.z = 0;
        // rebuild to enforce roll = 0
        phys->rotation = cm_quatFromEuler(euler);

        logs_log(LOG_DEBUG,
                 "Camera yaw %.2f° pitch %.2f° q(%.3f,%.3f,%.3f,%.3f)",
                 cm_rad2degf(euler.y), cm_rad2degf(euler.x),
                 phys->rotation.qx, phys->rotation.qy, phys->rotation.qz, phys->rotation.qw);
    }

    state->gui.mouse.dx = dx;
    state->gui.mouse.dy = dy;
    state->gui.mouse.x = xpos;
    state->gui.mouse.y = ypos;

    // logs_log(LOG_DEBUG, "Mouse position: (%lf, %lf) dx: %lf dy: %lf",
    //     state->gui.mouse.x, state->gui.mouse.y, state->gui.mouse.dx, state->gui.mouse.dy);
}

void mouse_posSet(State_t *state, Vec2f_t pos)
{
    glfwSetCursorPos(state->window.pWindow, (double)pos.x, (double)pos.y);
    // Reset delta calculations
    firstRecord = true;
}

void mouse_recenter(State_t *state)
{
    mouse_posSet(state, (Vec2f_t){state->window.frameBufferWidth / 2.0F, state->window.frameBufferHeight / 2.0F});
}

void mouse_init(State_t *state)
{
    glfwSetCursorPosCallback(state->window.pWindow, mouse_onMove);
}