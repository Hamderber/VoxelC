#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "cmath/cmath.h"
#include "rendering/camera/cameraController.h"

static bool firstRecord = true;
static double lastX = 0.0, lastY = 0.0;

void mouse_onMove(GLFWwindow *window, double xpos, double ypos)
{

    if (firstRecord)
    {
        firstRecord = false;
        lastX = xpos;
        lastY = ypos;
        return;
    }

    State_t *state = glfwGetWindowUserPointer(window);

    float dx = (float)(xpos - lastX);
    // invert Y for camera sense
    float dy = (float)(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    // Only update camera if not in any menus
    if (state->gui.menuDepth == 0)
    {
        camera_rotationUpdateNow(state);
    }

    state->gui.mouse.dx = dx;
    state->gui.mouse.dy = dy;
    state->gui.mouse.x = xpos;
    state->gui.mouse.y = ypos;
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