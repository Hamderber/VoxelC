#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "core/config.h"
#include "main.h"
#include "core/logs.h"
#include "cmath/cmath.h"
#include "cJSON.h"
#include "fileIO.h"
#include "input/types/inputActionMapping_t.h"
#include "core/types/state_t.h"
#include "input/types/input_t.h"
#include "input/types/defaulyKeyMapping_t.h"
#include "rendering/types/anisotropicFilteringOptions_t.h"

#define CFG_COMMENT "comment"
#define APP_CFG_WINDOW "window"
#define APP_CFG_WIDTH "width"
#define APP_CFG_HEIGHT "height"
#define APP_CFG_FULLSCREEN "fullscreen"
#define APP_CFG_MOUSE "mouse"
#define APP_CFG_RESET_MOUSE_CURSOR_ON_MENU_EXIT "resetCursorOnMenuExit"
#define APP_CFG_MOUSE_SENSITIVITY "mouseSensitivity"
#define APP_CFG_RENDERER "renderer"
#define APP_CFG_VSYNC "vsync"
#define APP_CFG_ANISOTROPY "anisotropy"
#define APP_CFG_FOV "fov"

typedef enum
{
    CONFIG_TYPE_APP,
    CONFIG_TYPE_KEYBINDINGS,
} ConfigType_t;

static const char *pCONFIG_FOLDER_NAME = "config";
static const char *pKEYBINDINGS_FILE_NAME = "keybindings.json";
static const char *pAPP_CONFIG_FILE_NAME = PROGRAM_NAME ".cfg.json";
static const double MOUSE_SENSITIVITY_MIN = 0.01;
static const double MOUSE_SENSITIVITY_MAX = 2.0;

static AppConfig_t s_AppConfig = {
    .pAPPLICATION_NAME = (PROGRAM_NAME " Application"),
    .pENGINE_NAME = (PROGRAM_NAME " Engine"),
    .pWINDOW_TITLE = PROGRAM_NAME,
    .windowWidth = 720,
    .windowHeight = 480,
    // If the window gets resized, the swapchain MUST be recreated
    .windowResizable = true,
    .windowFullscreen = false,
    // VK_CULL_MODE_BACK_BIT, enable this when performance is needed and meshing and stuff is known to work. (backface culling)
    // Triangles are drawn counter clockwise so clockwise triangles are "backwards"
    // VK_CULL_MODE_NONE Both purely for debugging to make sure meshing and whatnot works.
    .cullModeMask = VK_CULL_MODE_BACK_BIT,
    .maxFramesInFlight = 3,
    .vkAPIVersion = VK_API_VERSION_1_4,
    .swapchainComponentMapping = {
        // RGBA is still red/blue/green/alpha. Identity is keep it default but it could be .._A/etc
        // Identity = 0 so this could be omitted, but explicit declaration is better visually
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    },
    // default means that it will be auto-assigned during swapchain creation
    .swapchainBuffering = SWAPCHAIN_BUFFERING_DEFAULT,
    .vertexWindingDirection = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    // Same duration as Unity's fixedUpdate()
    .fixedTimeStep = 1.0 / 50.0,
    // Skip if more than x frames accumulated
    .maxPhysicsFrameDelay = 10,
#if defined(DEBUG)
    .vulkanValidation = true,
#else
    .vulkanValidation = false,
#endif
    .subtextureSize = 16,
    .vsync = true,
    .anisotropy = 16,
    .cameraFOV = 45.0F,
    .resetCursorOnMenuExit = true,
    .mouseSensitivity = 1.0,
    .atlasPaddingPx = 8,
};

static Input_t s_Input = {0};

static void config_input_buildDefault(void)
{
    // Apply defaults
    int key;
    for (key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        s_Input.pInputKeys[key] = (InputKey_t){
            .key = key,
            .keyDown = false,
            .keyUp = false,
            .pressedLastFrame = false,
            .pressedThisFrame = false,
            .inputMapping = INPUT_ACTION_UNMAPPED,
        };
    }

    InputActionMapping_t act;
    for (size_t i = 0; i < DEFAULT_KEY_MAPPINGS_COUNT; i++)
    {
        act = DEFAULT_KEY_MAPPINGS[i].action;
        key = DEFAULT_KEY_MAPPINGS[i].defaultKey;

        s_Input.pInputKeys[key].key = key;
        s_Input.pInputKeys[key].inputMapping = act;
    }
}

static cJSON *load_json_file(const char *pPATH, const char *pDEBUG_NAME)
{
    FILE *pFile = NULL;
    if (fileIO_file_open(&pFile, pPATH, "rb", pDEBUG_NAME) != FILE_IO_RESULT_SUCCESS)
    {
        logs_log(LOG_WARN, "Failed to open JSON file '%s' '%s'", pPATH, pDEBUG_NAME);
        return NULL;
    }

    // Seek to determine file length
    const long OFFSET = 0;
    if (fseek(pFile, OFFSET, SEEK_END) != 0)
    {
        logs_log(LOG_ERROR, "Failed to seek to end of file '%s' '%s'", pPATH, pDEBUG_NAME);
        fileIO_file_close(pFile, pDEBUG_NAME);
        return NULL;
    }

    long len = ftell(pFile);
    if (len < 0)
    {
        logs_log(LOG_ERROR, "Failed to get file position for '%s' '%s'", pPATH, pDEBUG_NAME);
        fileIO_file_close(pFile, pDEBUG_NAME);
        return NULL;
    }
    rewind(pFile);

    // Add 1 for the null terminator
    char *pBuffer = malloc(len + 1);
    if (!pBuffer)
    {
        logs_log(LOG_ERROR, "Failed to allocate %ld bytes for '%s' '%s'", len + 1, pPATH, pDEBUG_NAME);
        fileIO_file_close(pFile, pDEBUG_NAME);
        return NULL;
    }

    const size_t ELEMENT_SIZE = 1;
    size_t bytesRead = fread(pBuffer, ELEMENT_SIZE, len, pFile);
    logs_logIfError(bytesRead != (size_t)len, "Expected %ld bytes but read %zu bytes from '%s' '%s'",
                    len, bytesRead, pPATH, pDEBUG_NAME);

    pBuffer[len] = '\0';

    fileIO_file_close(pFile, pDEBUG_NAME);

    cJSON *pJson = cJSON_Parse(pBuffer);
    if (!pJson)
        logs_log(LOG_ERROR, "Failed to parse JSON contents from '%s' '%s'", pPATH, pDEBUG_NAME);

    free(pBuffer);
    return pJson;
}

static void config_keyBindings_save(const Input_t *pINPUT, cJSON *pRoot)
{
    // Iterate over all GLFW keycodes
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        InputActionMapping_t mapping = pINPUT->pInputKeys[key].inputMapping;

        // Only save mapped keys
        if (mapping != INPUT_ACTION_UNMAPPED)
        {
            const char *pACTION_NAME = INPUT_ACTION_MAPPING_NAMES[(int)mapping];
            cJSON_AddNumberToObject(pRoot, pACTION_NAME, key);
        }
    }
}

static void config_app_save(const AppConfig_t *pCFG, cJSON *pRoot)
{
    cJSON *pWindow = cJSON_AddObjectToObject(pRoot, APP_CFG_WINDOW);
    cJSON_AddNumberToObject(pWindow, APP_CFG_WIDTH, pCFG->windowWidth);
    cJSON_AddNumberToObject(pWindow, APP_CFG_HEIGHT, pCFG->windowHeight);
    cJSON_AddStringToObject(pWindow, CFG_COMMENT, "Should " PROGRAM_NAME " start in fullscreen mode?");
    cJSON_AddBoolToObject(pWindow, APP_CFG_FULLSCREEN, pCFG->windowFullscreen);

    cJSON *pMouse = cJSON_AddObjectToObject(pRoot, APP_CFG_MOUSE);
    cJSON_AddStringToObject(pMouse, CFG_COMMENT, "Reset the mouse cursor to the center of the screen when entering/exiting");
    cJSON_AddStringToObject(pMouse, CFG_COMMENT, "the first opened menu that causes the cursor to appear.");
    cJSON_AddBoolToObject(pMouse, APP_CFG_RESET_MOUSE_CURSOR_ON_MENU_EXIT, pCFG->resetCursorOnMenuExit);
    cJSON_AddStringToObject(pMouse, CFG_COMMENT, "Mouse sensitivity multiplier 0.01 to 2.0 where normal is 1.0");
    cJSON_AddNumberToObject(pMouse, APP_CFG_MOUSE_SENSITIVITY, pCFG->mouseSensitivity);

    cJSON *pRenderer = cJSON_AddObjectToObject(pRoot, APP_CFG_RENDERER);
    cJSON_AddBoolToObject(pRenderer, APP_CFG_VSYNC, pCFG->vsync);
    cJSON_AddNumberToObject(pRenderer, APP_CFG_ANISOTROPY, pCFG->anisotropy);
    cJSON_AddNumberToObject(pRenderer, APP_CFG_FOV, pCFG->cameraFOV);
}

static bool config_keyBindings_load(Input_t *pInput, const cJSON *pROOT)
{
    if (!pInput)
        return false;

    if (cJSON_IsObject(pROOT))
    {
        // Clear defaults
        for (int i = 0; i < DEFAULT_KEY_MAPPING_COUNT; i++)
            pInput->pInputKeys[DEFAULT_KEY_MAPPINGS[i].defaultKey].inputMapping = INPUT_ACTION_UNMAPPED;

        // Assign cfg keys. Skips over INPUT_ACTION_UNMAPPED
        for (int i = 1; i < INPUT_ACTION_COUNT; i++)
        {
            const char *pACTION_NAME = INPUT_ACTION_MAPPING_NAMES[i];
            cJSON *pKey = cJSON_GetObjectItemCaseSensitive(pROOT, pACTION_NAME);

            if (cJSON_IsNumber(pKey))
            {
                pInput->pInputKeys[pKey->valueint].inputMapping = (InputActionMapping_t)i;
                logs_log(LOG_DEBUG, "Keycode %3d -> %s", pKey->valueint, pACTION_NAME);
            }
            else if (i != INPUT_ACTION_UNMAPPED)
            {
                logs_log(LOG_WARN, "Failed to parse keycode assignment [%d] for %s. Using default.", i, pACTION_NAME);
                // Default key shift left 1 because UNMAPPED is a binding
                pInput->pInputKeys[DEFAULT_KEY_MAPPINGS[i - 1].defaultKey].inputMapping = (InputActionMapping_t)i;
            }
        }
    }
    else
        return false;

    return true;
}

static bool config_app_load(AppConfig_t *pCfg, const cJSON *pROOT)
{
    if (!pCfg)
        return false;

    cJSON *pWindow = cJSON_GetObjectItemCaseSensitive(pROOT, APP_CFG_WINDOW);
    if (cJSON_IsObject(pWindow))
    {
        cJSON *pWidth = cJSON_GetObjectItem(pWindow, APP_CFG_WIDTH);
        if (cJSON_IsNumber(pWidth))
            pCfg->windowWidth = pWidth->valueint;

        cJSON *pHeight = cJSON_GetObjectItem(pWindow, APP_CFG_HEIGHT);
        if (cJSON_IsNumber(pHeight))
            pCfg->windowHeight = pHeight->valueint;

        cJSON *pFullscreen = cJSON_GetObjectItem(pWindow, APP_CFG_FULLSCREEN);
        if (cJSON_IsBool(pFullscreen))
            pCfg->windowFullscreen = cJSON_IsTrue(pFullscreen);
    }
    else
        return false;

    cJSON *pMouse = cJSON_GetObjectItemCaseSensitive(pROOT, APP_CFG_MOUSE);
    if (cJSON_IsObject(pMouse))
    {
        cJSON *pResetCursorOnMenuExit = cJSON_GetObjectItem(pMouse, APP_CFG_RESET_MOUSE_CURSOR_ON_MENU_EXIT);
        if (cJSON_IsBool(pResetCursorOnMenuExit))
            pCfg->resetCursorOnMenuExit = cJSON_IsTrue(pResetCursorOnMenuExit);

        cJSON *pMouseSensitivity = cJSON_GetObjectItem(pMouse, APP_CFG_MOUSE_SENSITIVITY);
        if (cJSON_IsNumber(pMouseSensitivity))
            pCfg->mouseSensitivity = cmath_clampD(pMouseSensitivity->valuedouble, MOUSE_SENSITIVITY_MIN, MOUSE_SENSITIVITY_MAX);
    }
    else
        return false;

    cJSON *pRenderer = cJSON_GetObjectItemCaseSensitive(pROOT, APP_CFG_RENDERER);
    if (cJSON_IsObject(pRenderer))
    {
        cJSON *pVsync = cJSON_GetObjectItem(pRenderer, APP_CFG_VSYNC);
        if (cJSON_IsBool(pVsync))
            pCfg->vsync = cJSON_IsTrue(pVsync);

        cJSON *pAnisotropy = cJSON_GetObjectItem(pRenderer, APP_CFG_ANISOTROPY);
        if (cJSON_IsNumber(pAnisotropy))
            pCfg->anisotropy = pAnisotropy->valueint;

        cJSON *pFOV = cJSON_GetObjectItem(pRenderer, APP_CFG_FOV);
        if (cJSON_IsNumber(pFOV))
            pCfg->cameraFOV = (float)pFOV->valuedouble;
    }
    else
        return false;

    return true;
}

static void config_save(void *pCfg, const ConfigType_t TYPE)
{
    logs_logIfError(pCfg == NULL, "Attempted to save the config from an invalid pointer!");

    char pFullDir[MAX_DIR_PATH_LENGTH];
    if (fileIO_dir_create(pCONFIG_FOLDER_NAME, pFullDir) == FILE_IO_RESULT_DIR_CREATED)
        logs_log(LOG_WARN, "Directory '%s' not found at '%s'. Using defaults.", pCONFIG_FOLDER_NAME, pFullDir);

    cJSON *pRoot = cJSON_CreateObject();
    if (!pRoot)
    {
        logs_log(LOG_ERROR, "Failed to allocate JSON root!");
        return;
    }

    switch (TYPE)
    {
    case CONFIG_TYPE_APP:
        config_app_save((AppConfig_t *)pCfg, pRoot);
        break;
    case CONFIG_TYPE_KEYBINDINGS:
        config_keyBindings_save((Input_t *)pCfg, pRoot);
        break;
    }

    char *pJsonStr = cJSON_Print(pRoot);
    if (!pJsonStr)
    {
        logs_log(LOG_ERROR, "Failed to serialize JSON!");
        cJSON_Delete(pRoot);
        return;
    }

    const char *pFILE_NAME = NULL;
    switch (TYPE)
    {
    case CONFIG_TYPE_APP:
        pFILE_NAME = pAPP_CONFIG_FILE_NAME;
        break;
    case CONFIG_TYPE_KEYBINDINGS:
        pFILE_NAME = pKEYBINDINGS_FILE_NAME;
        break;
    }

    FILE *pFile = NULL;
    if (fileIO_file_create(&pFile, pCONFIG_FOLDER_NAME, pFILE_NAME) != FILE_IO_RESULT_FILE_CREATED)
    {
        logs_log(LOG_ERROR, "Failed to create config file '%s' in '%s'", pFILE_NAME, pCONFIG_FOLDER_NAME);
        free(pJsonStr);
        cJSON_Delete(pRoot);
        return;
    }

    fputs(pJsonStr, pFile);
    logs_log(LOG_DEBUG, "Saved '%s' to '%s/%s'", pFILE_NAME, pFullDir, pFILE_NAME);

    fileIO_file_close(pFile, pAPP_CONFIG_FILE_NAME);

    free(pJsonStr);
    cJSON_Delete(pRoot);
}

static bool config_json_load(cJSON **ppRoot, const char *pFULL_PATH, const char *pFILE_NAME)
{
    *ppRoot = load_json_file(pFULL_PATH, pFILE_NAME);
    if (!*ppRoot)
    {
        logs_log(LOG_ERROR, "Failed to read or parse '%s' JSON at '%s'. Using defaults.", pFILE_NAME, pFULL_PATH);

        // Attempt to rename the bad file
        char backupPath[MAX_DIR_PATH_LENGTH];
        bool isForFile = true;
        snprintf(backupPath, sizeof(backupPath), "%s.%s.old", pFULL_PATH, logs_timestampGet(isForFile));

        int renameResult = rename(pFULL_PATH, backupPath);
        if (renameResult == 0)
            logs_log(LOG_WARN, "Corrupted file '%s' renamed to '%s'", pFULL_PATH, backupPath);
        else
            logs_log(LOG_ERROR, "Failed to rename corrupted file'%s' to '%s' (error %d)", pFULL_PATH, backupPath, renameResult);

        return false;
    }

    return true;
}

static void config_load(void *pCfg, const ConfigType_t TYPE)
{
    if (!pCfg)
    {
        logs_log(LOG_ERROR, "Attempted to load config file for an invalid pointer!");
        return;
    }

    char pFullDir[MAX_DIR_PATH_LENGTH];
    if (fileIO_dir_create(pCONFIG_FOLDER_NAME, pFullDir) == FILE_IO_RESULT_DIR_CREATED)
        logs_log(LOG_WARN, "Directory '%s' not found at '%s'. Using defaults.", pCONFIG_FOLDER_NAME, pFullDir);

    char pFullPath[MAX_DIR_PATH_LENGTH];
    const char *pFILE_NAME = NULL;
    switch (TYPE)
    {
    case CONFIG_TYPE_APP:
        pFILE_NAME = pAPP_CONFIG_FILE_NAME;
        break;
    case CONFIG_TYPE_KEYBINDINGS:
        pFILE_NAME = pKEYBINDINGS_FILE_NAME;
        break;
    }

    // Build final file path (../folderName/fileName)
    snprintf(pFullPath, MAX_DIR_PATH_LENGTH, "%s/%s", pFullDir, pFILE_NAME);
    logs_log(LOG_DEBUG, "Attempting to load '%s' from '%s'", pFILE_NAME, pFullPath);

    cJSON *pRoot = NULL;

    do
    {
        if (config_json_load(&pRoot, pFullPath, pFILE_NAME))
            logs_log(LOG_DEBUG, "Loading existing %s json data...", pFILE_NAME);
        else
        {
            // Regenerate a fresh default config
            logs_log(LOG_DEBUG, "Regenerating new default config '%s'", pFILE_NAME);
            config_save(pCfg, TYPE);
            break;
        }

        bool readResult = false;
        switch (TYPE)
        {
        case CONFIG_TYPE_APP:
            readResult = config_app_load((AppConfig_t *)pCfg, pRoot);
            break;
        case CONFIG_TYPE_KEYBINDINGS:
            readResult = config_keyBindings_load((Input_t *)pCfg, pRoot);
            break;
        }

        if (!readResult)
        {
            logs_log(LOG_ERROR, "Failed to parse json for '%s'", pFILE_NAME);
            config_save(pCfg, TYPE);
        }
    } while (0);

    cJSON_Delete(pRoot);
}

static inline void *config_loadOrCreate(const ConfigType_t TYPE)
{
    switch (TYPE)
    {
    case CONFIG_TYPE_APP:
        config_load(&s_AppConfig, TYPE);
        return &s_AppConfig;
        break;
    case CONFIG_TYPE_KEYBINDINGS:
        config_input_buildDefault();
        config_load(&s_Input, TYPE);
        return &s_Input;
        break;
    }

    return NULL;
}

void config_init(struct State_t *pState)
{
    char fullDir[MAX_DIR_PATH_LENGTH];
    fileIO_dir_create(pCONFIG_FOLDER_NAME, fullDir);

    pState->config = *(AppConfig_t *)config_loadOrCreate(CONFIG_TYPE_APP);
    pState->input = *(Input_t *)config_loadOrCreate(CONFIG_TYPE_KEYBINDINGS);
}
