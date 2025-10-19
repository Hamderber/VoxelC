#include "core/logs.h"
#include "core/types/state_t.h"
#include "gui/guiState_t.h"
#include "gui/gui_t.h"
#include "events/eventBus.h"
#include "gui/mouse.h"
#include "gui/window.h"
#include "input/input.h"

void gui_toggleCursorCapture(State_t *state, bool isCaptured)
{
    glfwSetInputMode(state->window.pWindow, GLFW_CURSOR, isCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Recenter the mouse if it isn't captured. Currently this means that nested guis wont change the cursor position but exiting all
    // menus will
    if (state->config.resetCursorOnMenuExit && !isCaptured)
    {
        mouse_recenter(state);
    }

    logs_log(LOG_DEBUG, "GUI -> %s mouse cursor. Mouse position: (%lf, %lf) dx: %lf dy: %lf",
             isCaptured ? "captured" : "released", state->gui.mouse.x, state->gui.mouse.y, state->gui.mouse.dx, state->gui.mouse.dy);
}

void gui_publishChange(State_t *state, GUIMenuID_t id)
{
    logs_log(LOG_DEBUG, "Menu depth %d. Opened %s", (int)state->gui.menuDepth, GUI_MENU_NAMES[(int)id]);

    CtxGUI_t ctx = {
        .menuDepth = state->gui.menuDepth,
        .gui = &state->gui.GUIs[id],
    };

    // If there is no gui open (besides overlay) take the cursor back
    if (id == GUI_ID_NONE)
    {
        gui_toggleCursorCapture(state, true);
    }
    else
    {
        gui_toggleCursorCapture(state, false);
    }

    events_publish(state, &state->eventBus, EVENT_CHANNEL_GUI,
                   (Event_t){
                       .type = EVENT_TYPE_GUI,
                       .data.gui = &ctx,
                   });
}

void gui_onMenuEnter(State_t *state, GUIMenuID_t id)
{
    state->gui.guiStack[state->gui.menuDepth++] = id;

    gui_publishChange(state, id);
}

void gui_onMenuExit(State_t *state, GUIMenuID_t id)
{
    state->gui.guiStack[state->gui.menuDepth--] = GUI_ID_NONE;
    gui_publishChange(state, id);
}

void gui_depthToggle(State_t *state)
{
    // The overlay is the only thing open
    if (state->gui.menuDepth == 0)
    {
        gui_onMenuEnter(state, GUI_ID_PAUSE);
    }
    // Any other menu is open. Exit the top of the gui stack
    else
    {
        gui_onMenuExit(state, state->gui.guiStack[state->gui.menuDepth]);
    }
}

EventResult_t gui_onMenuTogglePress(struct State_t *state, Event_t *event, void *ctx)
{
    ctx = NULL;
    if (event == NULL)
    {
        return EVENT_RESULT_ERROR;
    }

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_START)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_MENU_TOGGLE:
                    logs_log(LOG_DEBUG, "Menu toggle (pressed)");
                    gui_depthToggle(state);
                    return EVENT_RESULT_CONSUME;
                    break;
                }
            }
        }
    }

    return EVENT_RESULT_PASS;
}

EventResult_t gui_onFullscreenTogglePress(struct State_t *state, Event_t *event, void *ctx)
{
    ctx = NULL;
    if (event == NULL)
    {
        return EVENT_RESULT_ERROR;
    }

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_START)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_FULLSCREEN_TOGGLE:
                    logs_log(LOG_DEBUG, "Fullscreen toggle (pressed)");
                    win_fullscreenToggle(state, !state->window.fullscreen);
                    return EVENT_RESULT_CONSUME;
                    break;
                }
            }
        }
    }

    return EVENT_RESULT_PASS;
}

void gui_buildGUIs(State_t *state)
{
    for (size_t i = 0; i < GUI_ID_COUNT; i++)
    {
        GUIMenuID_t id = (GUIMenuID_t)i;

        state->gui.GUIs[i] = (GUI_t){
            // Overlay gui is always active
            .active = id == GUI_ID_OVERLAY,
            .id = id,
        };

        // Getting the values from state directly to make sure assignment worked
        logs_log(LOG_DEBUG, "GUI Menu %s created. Active = %s",
                 GUI_MENU_NAMES[state->gui.GUIs[i].id], state->gui.GUIs[i].active ? "true" : "false");
    }
}

void gui_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing GUI Controller...");

    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, gui_onMenuTogglePress, false, false, NULL);
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, gui_onFullscreenTogglePress, false, false, NULL);

    // Don't start with the cursor captured. Start in the pause menu
    bool centerCursor = state->config.resetCursorOnMenuExit;
    state->config.resetCursorOnMenuExit = false;
    gui_toggleCursorCapture(state, false);
    input_inputActionSimulate(state, INPUT_ACTION_MENU_TOGGLE, CTX_INPUT_ACTION_START);
    state->config.resetCursorOnMenuExit = centerCursor;
}