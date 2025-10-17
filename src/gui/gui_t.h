#pragma once

typedef enum
{
    GUI_ID_NONE,
    GUI_ID_OVERLAY,
    GUI_ID_PAUSE,
    GUI_ID_COUNT,
} GUIMenuID_t;

static const char *GUI_MENU_NAMES[] = {
    "GUI_ID_NONE",
    "GUI_ID_OVERLAY",
    "GUI_ID_PAUSE",
    "GUI_ID_COUNT",
};

typedef struct
{
    GUIMenuID_t id;
    bool active;
} GUI_t;