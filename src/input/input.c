#pragma region Includes
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
#include "input.h"
#pragma endregion
#pragma region Key Names
static const char *input_keyNameGet(const int KEY)
{
    switch (KEY)
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
#pragma endregion
#pragma region Events
void input_inputAction_simulate(State_t *pState, InputActionMapping_t inputActionMapped, CtxDescriptor_t actionState)
{
    if (!pState)
    {
        logs_log(LOG_ERROR, "Recieved an invalid refrence to state!");
        return;
    }

    InputAction_t inputAction = {
        .action = inputActionMapped,
        .actionState = actionState,
    };

    CtxInputMapped_t data = {
        .actionCount = 1,
        .inputActions = &inputAction,
    };

    Event_t event = {
        .type = EVENT_TYPE_INPUT_MAPPED,
        .data.inputMapped = &data,
    };

    logs_log(LOG_DEBUG, "Simulating the input action %s -> %s", INPUT_ACTION_MAPPING_NAMES[(int)inputActionMapped],
             actionState == CTX_INPUT_ACTION_START ? "pressed" : "released");

    events_publish(pState, &pState->eventBus, EVENT_CHANNEL_INPUT, event);
}

/// @brief Publishes this frame's keystrokes to the state's event system
static void keystrokes_publish(State_t *pState, int *pStrokeCount, Keystroke_t *pKeystrokes)
{
    if (!pState || !pStrokeCount || !pKeystrokes)
        return;

    CtxInputRaw_t ctxRaw = {
        .keyCount = *pStrokeCount,
        .keystrokes = pKeystrokes,
    };

    events_publish(pState, &pState->eventBus, EVENT_CHANNEL_INPUT,
                   (Event_t){
                       .type = EVENT_TYPE_INPUT_RAW,
                       .data.inputRaw = &ctxRaw,
                   });

    memset(pKeystrokes, 0, sizeof(pKeystrokes));
}

static void inputActions_publish(State_t *pState, int *pActionCount, InputAction_t *pInputActions)
{
    if (!pState || !pActionCount || !pInputActions)
        return;

    CtxInputMapped_t ctxMapped = {
        .actionCount = *pActionCount,
        .inputActions = pInputActions,
    };

    events_publish(pState, &pState->eventBus, EVENT_CHANNEL_INPUT,
                   (Event_t){
                       .type = EVENT_TYPE_INPUT_MAPPED,
                       .data.inputMapped = &ctxMapped,
                   });

    memset(pInputActions, 0, sizeof(pInputActions));
}
#pragma endregion
#pragma region Keys
/// @brief Handle key being released
static void key_onRelease(State_t *pState, int *pStrokeCount, int *pActionCount,
                          InputAction_t *pInputActions, Keystroke_t *pKeystrokes, const int KEYCODE)
{
    if (!pState || !pStrokeCount || !pActionCount || !pInputActions || !pKeystrokes)
        return;
    if (!pState->input.pInputKeys)
        return;

    InputKey_t *pKey = &pState->input.pInputKeys[KEYCODE];

#if defined(DEBUG)
    const char *pKEY_NAME = input_keyNameGet(KEYCODE);
    const char *pACTION_NAME = INPUT_ACTION_MAPPING_NAMES[(int)pKey->inputMapping];
    logs_log(LOG_DEBUG, "Key %3d (%-12s) up -> Action: %s", KEYCODE, pKEY_NAME, pACTION_NAME);
#endif

    pKey->keyDown = false;
    pKey->keyUp = true;

    pKeystrokes[(*pStrokeCount)++] = (Keystroke_t){
        .direction = CTX_KEYSTROKE_UP,
        .keyCode = KEYCODE,
    };

    if (input_key_HasInputActionMapping(pState, KEYCODE))
        pInputActions[(*pActionCount)++] = (InputAction_t){
            .action = pKey->inputMapping,
            .actionState = CTX_INPUT_ACTION_END,
        };
}

/// @brief Handle key being pressed
static void key_onPress(State_t *pState, int *pStrokeCount, int *pActionCount,
                        InputAction_t *pInputActions, Keystroke_t *pKeystrokes, const int KEYCODE)
{
    if (!pState || !pStrokeCount || !pActionCount || !pInputActions || !pKeystrokes)
        return;
    if (!pState->input.pInputKeys)
        return;

    InputKey_t *pKey = &pState->input.pInputKeys[KEYCODE];

#if defined(DEBUG)
    const char *pKEY_NAME = input_keyNameGet(KEYCODE);
    const char *pACTION_NAME = INPUT_ACTION_MAPPING_NAMES[(int)pKey->inputMapping];
    logs_log(LOG_DEBUG, "Key %3d (%-12s) down-> Action: %s", KEYCODE, pKEY_NAME, pACTION_NAME);
#endif

    pKey->keyUp = false;
    pKey->keyDown = true;

    pKeystrokes[(*pStrokeCount)++] = (Keystroke_t){
        .direction = CTX_KEYSTROKE_DOWN,
        .keyCode = KEYCODE,
    };

    if (input_key_HasInputActionMapping(pState, KEYCODE))
        pInputActions[(*pActionCount)++] = (InputAction_t){
            .action = pKey->inputMapping,
            .actionState = CTX_INPUT_ACTION_START,
        };
}

/// @brief Query the keycode for being pressed or released. Handles the associated state accordingly.
static void poll_key(State_t *pState, int *pStrokeCount, int *pActionCount,
                     InputAction_t *pInputActions, Keystroke_t *pKeystrokes, const int KEYCODE)
{
    if (!pState || !pState->input.pInputKeys)
        return;

    GLFWwindow *pWindow = pState->window.pWindow;
    InputKey_t *pKey = &pState->input.pInputKeys[KEYCODE];

    if (!pWindow || !pKey || !pStrokeCount || !pActionCount || !pInputActions || !pKeystrokes)
        return;

    // Query GLFW key state
    bool isPressed = glfwGetKey(pWindow, KEYCODE) == GLFW_PRESS;

    // Update pressed states
    pKey->pressedLastFrame = pKey->pressedThisFrame;
    pKey->pressedThisFrame = isPressed;

    // Only act on transitions
    if (pKey->pressedThisFrame && !pKey->pressedLastFrame)
        key_onPress(pState, pStrokeCount, pActionCount, pInputActions, pKeystrokes, KEYCODE);
    else if (!pKey->pressedThisFrame && pKey->pressedLastFrame)
        key_onRelease(pState, pStrokeCount, pActionCount, pInputActions, pKeystrokes, KEYCODE);
}
#pragma endregion
#pragma region Polling
void input_poll(State_t *pState)
{
    if (!pState)
    {
        logs_log(LOG_ERROR, "Recieved an invalid refrence to state!");
        return;
    }

    // Track all keys pressed for this frame. There are 104 keys on a US keyboard so 120 is safe for special keys too.
    // Honestly pressing all keys at once deserves a crash anyway imo.
    int strokeCount = 0;
    static Keystroke_t pKeystrokes[120];

    int actionCount = 0;
    // This size is arbitrary right now. We'll see what happens. Since this is per-frame, the value can be relatively small
    static InputAction_t pInputActions[32];

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
        poll_key(pState, &strokeCount, &actionCount, pInputActions, pKeystrokes, key);

    if (strokeCount > 0)
        keystrokes_publish(pState, &strokeCount, pKeystrokes);

    if (actionCount > 0)
        inputActions_publish(pState, &actionCount, pInputActions);
}
#pragma endregion
#pragma region Init
void input_init(const State_t *pSTATE)
{
#if defined(DEBUG)
    logs_log(LOG_DEBUG, "Initializing input system...");
    // Log mapped keys. Actual mapping is handled by loading the keybindings cfg
    InputActionMapping_t mapping = INPUT_ACTION_UNMAPPED;
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        const char *pKEY_NAME = input_keyNameGet(key);

        if (!pKEY_NAME || strcmp(pKEY_NAME, "Unknown") == 0)
            continue;

        mapping = pSTATE->input.pInputKeys[key].inputMapping;
        const char *pMAPPING_NAME = INPUT_ACTION_MAPPING_NAMES[(int)mapping];

        if (mapping != INPUT_ACTION_UNMAPPED)
            logs_log(LOG_DEBUG, "Keycode %3d (%-12s) -> %s", key, pKEY_NAME, pMAPPING_NAME);
    }
#else
    pSTATE;
#endif
}
#pragma endregion