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
    state->gui.mouse.dx = xpos - lastX;
    // Y is inverse because the window tracks from top to bottom but dx/dy is left to right and down to up (normal graphing)
    state->gui.mouse.dy = lastY - ypos;

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