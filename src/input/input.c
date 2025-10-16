#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "core/logs.h"
#include "input/types/inputActionMapping_t.h"
#include "input/types/inputKey_t.h"
#include <string.h>
#include "events/eventBus.h"
#include "events/context/CtxInputMapped_t.h"
#include "events/context/CtxInputRaw_t.h"
#include "events/context/CtxDescriptor_t.h"

const char *input_keyNameGet(int key)
{
    switch (key)
    {
    case GLFW_KEY_SPACE:
        return "Space";
    case GLFW_KEY_APOSTROPHE:
        return "'";
    case GLFW_KEY_COMMA:
        return ",";
    case GLFW_KEY_MINUS:
        return "-";
    case GLFW_KEY_PERIOD:
        return ".";
    case GLFW_KEY_SLASH:
        return "/";
    case GLFW_KEY_0:
        return "0";
    case GLFW_KEY_1:
        return "1";
    case GLFW_KEY_2:
        return "2";
    case GLFW_KEY_3:
        return "3";
    case GLFW_KEY_4:
        return "4";
    case GLFW_KEY_5:
        return "5";
    case GLFW_KEY_6:
        return "6";
    case GLFW_KEY_7:
        return "7";
    case GLFW_KEY_8:
        return "8";
    case GLFW_KEY_9:
        return "9";
    case GLFW_KEY_SEMICOLON:
        return ";";
    case GLFW_KEY_EQUAL:
        return "=";
    case GLFW_KEY_A:
        return "A";
    case GLFW_KEY_B:
        return "B";
    case GLFW_KEY_C:
        return "C";
    case GLFW_KEY_D:
        return "D";
    case GLFW_KEY_E:
        return "E";
    case GLFW_KEY_F:
        return "F";
    case GLFW_KEY_G:
        return "G";
    case GLFW_KEY_H:
        return "H";
    case GLFW_KEY_I:
        return "I";
    case GLFW_KEY_J:
        return "J";
    case GLFW_KEY_K:
        return "K";
    case GLFW_KEY_L:
        return "L";
    case GLFW_KEY_M:
        return "M";
    case GLFW_KEY_N:
        return "N";
    case GLFW_KEY_O:
        return "O";
    case GLFW_KEY_P:
        return "P";
    case GLFW_KEY_Q:
        return "Q";
    case GLFW_KEY_R:
        return "R";
    case GLFW_KEY_S:
        return "S";
    case GLFW_KEY_T:
        return "T";
    case GLFW_KEY_U:
        return "U";
    case GLFW_KEY_V:
        return "V";
    case GLFW_KEY_W:
        return "W";
    case GLFW_KEY_X:
        return "X";
    case GLFW_KEY_Y:
        return "Y";
    case GLFW_KEY_Z:
        return "Z";
    case GLFW_KEY_LEFT_BRACKET:
        return "[";
    case GLFW_KEY_BACKSLASH:
        return "\\";
    case GLFW_KEY_RIGHT_BRACKET:
        return "]";
    case GLFW_KEY_GRAVE_ACCENT:
        return "`";
    case GLFW_KEY_WORLD_1:
        return "World1";
    case GLFW_KEY_WORLD_2:
        return "World2";

    case GLFW_KEY_ESCAPE:
        return "Escape";
    case GLFW_KEY_ENTER:
        return "Enter";
    case GLFW_KEY_TAB:
        return "Tab";
    case GLFW_KEY_BACKSPACE:
        return "Backspace";
    case GLFW_KEY_INSERT:
        return "Insert";
    case GLFW_KEY_DELETE:
        return "Delete";
    case GLFW_KEY_RIGHT:
        return "Right";
    case GLFW_KEY_LEFT:
        return "Left";
    case GLFW_KEY_DOWN:
        return "Down";
    case GLFW_KEY_UP:
        return "Up";
    case GLFW_KEY_PAGE_UP:
        return "PageUp";
    case GLFW_KEY_PAGE_DOWN:
        return "PageDown";
    case GLFW_KEY_HOME:
        return "Home";
    case GLFW_KEY_END:
        return "End";

    case GLFW_KEY_CAPS_LOCK:
        return "CapsLock";
    case GLFW_KEY_SCROLL_LOCK:
        return "ScrollLock";
    case GLFW_KEY_NUM_LOCK:
        return "NumLock";
    case GLFW_KEY_PRINT_SCREEN:
        return "PrintScreen";
    case GLFW_KEY_PAUSE:
        return "Pause";

    case GLFW_KEY_F1:
        return "F1";
    case GLFW_KEY_F2:
        return "F2";
    case GLFW_KEY_F3:
        return "F3";
    case GLFW_KEY_F4:
        return "F4";
    case GLFW_KEY_F5:
        return "F5";
    case GLFW_KEY_F6:
        return "F6";
    case GLFW_KEY_F7:
        return "F7";
    case GLFW_KEY_F8:
        return "F8";
    case GLFW_KEY_F9:
        return "F9";
    case GLFW_KEY_F10:
        return "F10";
    case GLFW_KEY_F11:
        return "F11";
    case GLFW_KEY_F12:
        return "F12";
    case GLFW_KEY_F13:
        return "F13";
    case GLFW_KEY_F14:
        return "F14";
    case GLFW_KEY_F15:
        return "F15";
    case GLFW_KEY_F16:
        return "F16";
    case GLFW_KEY_F17:
        return "F17";
    case GLFW_KEY_F18:
        return "F18";
    case GLFW_KEY_F19:
        return "F19";
    case GLFW_KEY_F20:
        return "F20";
    case GLFW_KEY_F21:
        return "F21";
    case GLFW_KEY_F22:
        return "F22";
    case GLFW_KEY_F23:
        return "F23";
    case GLFW_KEY_F24:
        return "F24";
    case GLFW_KEY_F25:
        return "F25";

    case GLFW_KEY_KP_0:
        return "NumPad0";
    case GLFW_KEY_KP_1:
        return "NumPad1";
    case GLFW_KEY_KP_2:
        return "NumPad2";
    case GLFW_KEY_KP_3:
        return "NumPad3";
    case GLFW_KEY_KP_4:
        return "NumPad4";
    case GLFW_KEY_KP_5:
        return "NumPad5";
    case GLFW_KEY_KP_6:
        return "NumPad6";
    case GLFW_KEY_KP_7:
        return "NumPad7";
    case GLFW_KEY_KP_8:
        return "NumPad8";
    case GLFW_KEY_KP_9:
        return "NumPad9";
    case GLFW_KEY_KP_DECIMAL:
        return "NumPad.";
    case GLFW_KEY_KP_DIVIDE:
        return "NumPad/";
    case GLFW_KEY_KP_MULTIPLY:
        return "NumPad*";
    case GLFW_KEY_KP_SUBTRACT:
        return "NumPad-";
    case GLFW_KEY_KP_ADD:
        return "NumPad+";
    case GLFW_KEY_KP_ENTER:
        return "NumPadEnter";
    case GLFW_KEY_KP_EQUAL:
        return "NumPad=";

    case GLFW_KEY_LEFT_SHIFT:
        return "LeftShift";
    case GLFW_KEY_RIGHT_SHIFT:
        return "RightShift";
    case GLFW_KEY_LEFT_CONTROL:
        return "LeftCtrl";
    case GLFW_KEY_RIGHT_CONTROL:
        return "RightCtrl";
    case GLFW_KEY_LEFT_ALT:
        return "LeftAlt";
    case GLFW_KEY_RIGHT_ALT:
        return "RightAlt";
    case GLFW_KEY_LEFT_SUPER:
        return "LeftSuper";
    case GLFW_KEY_RIGHT_SUPER:
        return "RightSuper";
    case GLFW_KEY_MENU:
        return "Menu";

    default:
        return "Unknown";
    }
}

InputActionMapping_t input_keyActionGet(State_t *state, int key)
{
    return state->window.input.pInputKeys[key].inputMapping;
}

/// @brief Checks if the keycode is mapped to INPUT_ACTION_UNMAPPED within the state
/// @param state
/// @param key
/// @return bool
bool input_keyHasInputAction(State_t *state, int key)
{
    return state->window.input.pInputKeys[key].inputMapping != INPUT_ACTION_UNMAPPED;
}

void input_update(State_t *state)
{
    GLFWwindow *window = state->window.pWindow;
    Input_t *input = &state->window.input;

    // Track all keys pressed for this frame. There are 104 keys on a US keyboard so 120 is safe for special keys too.
    // Honestly pressing all keys at once deserves a crash anyway imo.
    static int strokeIndex = 0;
    static int strokeCount = 0;
    static Keystroke_t keystrokes[120];

    static int actionIndex = 0;
    static int actionCount = 0;
    // This size is arbitrary right now. We'll see what happens. Since this is per-frame, the value can be relatively small
    static InputAction_t inputActions[32];

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        InputKey_t *k = &input->pInputKeys[key];

        // Query GLFW key state
        bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;

        // Update pressed states
        k->pressedLastFrame = k->pressedThisFrame;
        k->pressedThisFrame = isPressed;

        // Only act on transitions
        if (k->pressedThisFrame && !k->pressedLastFrame)
        {
            const char *keyName = input_keyNameGet(key);
            const char *actionName = INPUT_ACTION_MAPPING_NAMES[(int)k->inputMapping];

            k->keyUp = false;
            k->keyDown = true;

            logs_log(LOG_DEBUG, "Key %3d (%-12s) down-> Action: %s", key, keyName, actionName);

            keystrokes[strokeIndex++] = (Keystroke_t){
                .direction = CTX_KEYSTROKE_DOWN,
                .keyCode = key,
            };
            strokeCount++;

            if (input_keyHasInputAction(state, key))
            {
                inputActions[actionIndex++] = (InputAction_t){
                    .action = k->inputMapping,
                    .actionState = CTX_INPUT_ACTION_START,
                };
            }
            actionCount++;
        }
        else if (!k->pressedThisFrame && k->pressedLastFrame)
        {
            const char *keyName = input_keyNameGet(key);
            const char *actionName = INPUT_ACTION_MAPPING_NAMES[(int)k->inputMapping];

            k->keyDown = false;
            k->keyUp = true;

            logs_log(LOG_DEBUG, "Key %3d (%-12s) up -> Action: %s", key, keyName, actionName);

            keystrokes[strokeIndex++] = (Keystroke_t){
                .direction = CTX_KEYSTROKE_UP,
                .keyCode = key,
            };
            strokeCount++;

            if (input_keyHasInputAction(state, key))
            {
                inputActions[actionIndex++] = (InputAction_t){
                    .action = k->inputMapping,
                    .actionState = CTX_INPUT_ACTION_END,
                };
            }
            actionCount++;
        }
    }

    if (strokeIndex > 0)
    {
        CtxInputRaw_t ctxRaw = {
            .keyCount = strokeCount,
            .keystrokes = keystrokes,
        };

        events_publish(state, &state->eventBus, EVENT_CHANNEL_INPUT,
                       (Event_t){
                           .type = EVENT_TYPE_INPUT_RAW,
                           .data.inputRaw = &ctxRaw,
                       });

        strokeIndex = 0;
        strokeCount = 0;
        memset(keystrokes, 0, sizeof(keystrokes));
    }

    if (actionIndex > 0)
    {
        CtxInputMapped_t ctxMapped = {
            .actionCount = actionCount,
            .inputActions = inputActions,
        };

        events_publish(state, &state->eventBus, EVENT_CHANNEL_INPUT,
                       (Event_t){
                           .type = EVENT_TYPE_INPUT_MAPPED,
                           .data.inputMapped = &ctxMapped,
                       });

        actionIndex = 0;
        actionCount = 0;
        memset(inputActions, 0, sizeof(inputActions));
    }
}

void input_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing input system...");

    state->window.input = cfg_inputInit();

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        const char *keyName = input_keyNameGet(key);

        // Skip unknown keys entirely
        if (!keyName || strcmp(keyName, "Unknown") == 0)
            continue;

        // Pull the configured mapping for this key
        InputActionMapping_t mapping = state->window.input.pInputKeys[key].inputMapping;
        const char *mappingName = INPUT_ACTION_MAPPING_NAMES[(int)mapping];

        // Assign a fully initialized key struct
        state->window.input.pInputKeys[key] = (InputKey_t){
            .key = key,
            .keyDown = false,
            .keyUp = false,
            .pressedLastFrame = false,
            .pressedThisFrame = false,
            .inputMapping = mapping,
        };

        if (mapping != INPUT_ACTION_UNMAPPED)
        {
            logs_log(LOG_DEBUG, "Keycode %3d (%-12s) -> %s", key, keyName, mappingName);
        }
    }
}