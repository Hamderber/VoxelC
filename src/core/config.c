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
#include "input/types/input_t.h"
#include "input/types/defaulyKeyMapping_t.h"

static FILE *pKeyCfg;
const char *cfgFolder = "config";
static FILE *pCfg;

#define KEYBINDINGS_FILE_NAME "keybindings.json"
#define APP_CONFIG_NAME PROGRAM_NAME ".cfg.json"

static AppConfig_t appConfig = {
    .pApplicationName = (PROGRAM_NAME " Application"),
    .pEngineName = (PROGRAM_NAME " Engine"),
    .pWindowTitle = PROGRAM_NAME,
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
    .vulkanValidation = true,
    .subtextureSize = 16,
    .vsync = true,
    .anisotropy = 16,
    .cameraFOV = 45.0F,
    .resetCursorOnMenuExit = true,
    .mouseSensitivity = 1.0,
};

static cJSON *load_json_file(const char *path, const char *debugName)
{
    // Open file safely with logging
    FILE *file = file_open(path, "rb", debugName);
    if (!file)
    {
        logs_log(LOG_ERROR, "Failed to open JSON file '%s' '%s'", path, debugName);
        return NULL;
    }

    // Seek to determine file length
    if (fseek(file, 0, SEEK_END) != 0)
    {
        logs_log(LOG_ERROR, "Failed to seek to end of file '%s' '%s'", path, debugName);
        file_close(file, debugName);
        return NULL;
    }

    long len = ftell(file);
    if (len < 0)
    {
        logs_log(LOG_ERROR, "ftell() failed for '%s' '%s'", path, debugName);
        file_close(file, debugName);
        return NULL;
    }
    rewind(file);

    // Allocate buffer
    char *buffer = malloc(len + 1);
    if (!buffer)
    {
        logs_log(LOG_ERROR, "Failed to allocate %ld bytes for '%s' '%s'", len + 1, path, debugName);
        file_close(file, debugName);
        return NULL;
    }

    // Read entire file
    size_t bytesRead = fread(buffer, 1, len, file);
    if (bytesRead != (size_t)len)
    {
        logs_log(LOG_WARN, "Expected %ld bytes but read %zu bytes from '%s' '%s'", len, bytesRead, path, debugName);
    }

    buffer[len] = '\0';

    // Close file safely
    file_close(file, debugName);

    // Parse JSON
    cJSON *json = cJSON_Parse(buffer);
    if (!json)
    {
        logs_log(LOG_ERROR, "Failed to parse JSON contents from '%s' '%s'", path, debugName);
    }

    free(buffer);
    return json;
}

void cfg_keyBindingsSave(Input_t *input, const char *dir, const char *filename)
{
    char path[MAX_DIR_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        logs_log(LOG_ERROR, "Failed to allocate JSON root for keybinding save");
        return;
    }

    // Iterate over all GLFW keycodes
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
    {
        InputActionMapping_t mapping = input->pInputKeys[key].inputMapping;

        // Only save mapped keys
        if (mapping != INPUT_ACTION_UNMAPPED)
        {
            const char *actionName = INPUT_ACTION_MAPPING_NAMES[(int)mapping];
            cJSON_AddNumberToObject(root, actionName, key);
        }
    }

    char *jsonStr = cJSON_Print(root);
    if (!jsonStr)
    {
        logs_log(LOG_ERROR, "Failed to serialize keybinding JSON");
        cJSON_Delete(root);
        return;
    }

    FILE *file = file_create(dir, filename);
    if (!file)
    {
        logs_log(LOG_ERROR, "Failed to open keybindings file for writing: %s", path);
        free(jsonStr);
        cJSON_Delete(root);
        return;
    }

    fputs(jsonStr, file);
    file_close(file, filename);
    free(jsonStr);
    cJSON_Delete(root);

    logs_log(LOG_DEBUG, "Saved keybindings to '%s'", path);
}

void cfg_keyBindingsLoad(Input_t *input, const char *dir, const char *fileName)
{
    if (!input)
    {
        logs_log(LOG_ERROR, "Input_t pointer was NULL in cfg_keyBindingsLoad()");
        return;
    }

    char fullDir[MAX_DIR_PATH_LENGTH];
    if (!file_dirExists(dir, fullDir))
    {
        logs_log(LOG_WARN, "Directory '%s' not found at '%s'. Using default keybindings.", dir, fullDir);
        return;
    }

    // Build final file path (../dir/fileName)
    char fullPath[MAX_DIR_PATH_LENGTH];
    snprintf(fullPath, MAX_DIR_PATH_LENGTH, "%s/%s", fullDir, fileName);
    logs_log(LOG_DEBUG, "Attempting to load '%s' from '%s'", fileName, fullPath);

    cJSON *root = load_json_file(fullPath, fileName);
    if (!root)
    {
        logs_log(LOG_ERROR, "Failed to read or parse '%s' JSON at '%s'. Using defaults.", fileName, fullPath);

        // Attempt to rename the bad file
        char backupPath[MAX_DIR_PATH_LENGTH];
        snprintf(backupPath, sizeof(backupPath), "%s.old", fullPath);

        int renameResult = rename(fullPath, backupPath);
        if (renameResult == 0)
        {
            logs_log(LOG_WARN, "Corrupted keybinding file '%s' renamed to '%s'", fullPath, backupPath);
        }
        else
        {
            logs_log(LOG_ERROR, "Failed to rename corrupted keybinding file '%s' to '%s' (error %d)", fullPath, backupPath, renameResult);
        }

        // Regenerate a fresh default keybinding file
        logs_log(LOG_DEBUG, "Regenerating new default keybinding file '%s'", fileName);
        cfg_keyBindingsSave(input, dir, fileName);
        return;
    }

    // Parse the JSON contents
    cJSON *bindings = root;
    if (cJSON_IsObject(bindings))
    {
        for (int i = 0; i < INPUT_ACTION_COUNT; i++)
        {
            const char *actionName = INPUT_ACTION_MAPPING_NAMES[i];
            cJSON *keyItem = cJSON_GetObjectItemCaseSensitive(bindings, actionName);

            if (cJSON_IsNumber(keyItem))
            {
                input->pInputKeys[i].key = keyItem->valueint;
                input->pInputKeys[i].inputMapping = (InputActionMapping_t)i;
            }
        }
    }

    logs_log(LOG_DEBUG, "Loaded '%s' keybindings from '%s'", fileName, fullPath);
    cJSON_Delete(root);
}

Input_t cfg_inputDefaults(void)
{
    Input_t input = {0};

    // Apply defaults
    for (size_t i = 0; i < DEFAULT_KEY_MAPPINGS_COUNT; i++)
    {
        InputActionMapping_t act = DEFAULT_KEY_MAPPINGS[i].action;
        int key = DEFAULT_KEY_MAPPINGS[i].defaultKey;

        input.pInputKeys[key].key = key;
        input.pInputKeys[key].inputMapping = act;
        input.pInputKeys[key].pressedLastFrame = false;
        input.pInputKeys[key].pressedThisFrame = false;
    }

    return input;
}

Input_t cfg_keyBindingsLoadOrCreate(void)
{
    Input_t input = cfg_inputDefaults();

    char fullDir[MAX_DIR_PATH_LENGTH];

    if (file_exists(cfgFolder, KEYBINDINGS_FILE_NAME, fullDir))
    {
        logs_log(LOG_DEBUG, "Loading existing keybindings");
        cfg_keyBindingsLoad(&input, cfgFolder, KEYBINDINGS_FILE_NAME);
    }
    else
    {
        logs_log(LOG_WARN, "Unable to locate keybinding file. Creating default.");

        cfg_keyBindingsSave(&input, cfgFolder, KEYBINDINGS_FILE_NAME);
    }

    return input;
}

void cfg_keyBindingsDestroy(void)
{
    if (pKeyCfg)
    {
        file_close(pKeyCfg, KEYBINDINGS_FILE_NAME);
        pKeyCfg = NULL;
    }
}

void cfg_appSave(const AppConfig_t *cfg, const char *dir, const char *fileName)
{
    if (!cfg)
    {
        logs_log(LOG_ERROR, "AppConfig_t pointer was NULL in cfg_appSave()");
        return;
    }

    // Ensure directory exists
    char fullDir[MAX_DIR_PATH_LENGTH];
    logs_log(LOG_DEBUG, "Verifying '%s' directory still exists.", dir);

    if (!file_dirExists(dir, fullDir))
    {
        logs_log(LOG_ERROR, "Failed to ensure '%s' directory exists at '%s'", dir, fullDir);
        return;
    }

    // Create JSON root
    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        logs_log(LOG_ERROR, "Failed to allocate JSON root for cfg_appSave()");
        return;
    }

    cJSON *window = cJSON_AddObjectToObject(root, "window");
    cJSON_AddNumberToObject(window, "width", cfg->windowWidth);
    cJSON_AddNumberToObject(window, "height", cfg->windowHeight);
    cJSON_AddStringToObject(window, "comment", "Should " PROGRAM_NAME " start in fullscreen mode?");
    cJSON_AddBoolToObject(window, "fullscreen", cfg->windowFullscreen);

    cJSON *mouse = cJSON_AddObjectToObject(root, "mouse");
    cJSON_AddStringToObject(mouse, "comment", "Reset the mouse cursor to the center of the screen when entering/exiting");
    cJSON_AddStringToObject(mouse, "comment", "the first opened menu that causes the cursor to appear.");
    cJSON_AddBoolToObject(mouse, "resetCursorOnMenuExit", cfg->resetCursorOnMenuExit);
    cJSON_AddStringToObject(mouse, "comment", "Mouse sensitivity multiplier 0.01 to 2.0 where normal is 1.0");
    cJSON_AddNumberToObject(mouse, "mouseSensitivity", cfg->mouseSensitivity);

    cJSON *renderer = cJSON_AddObjectToObject(root, "renderer");
    cJSON_AddBoolToObject(renderer, "vsync", cfg->vsync);
    cJSON_AddNumberToObject(renderer, "anisotropy", cfg->anisotropy);
    cJSON_AddNumberToObject(renderer, "fov", cfg->cameraFOV);

    // Serialize JSON
    char *json_str = cJSON_Print(root);
    if (!json_str)
    {
        logs_log(LOG_ERROR, "Failed to serialize JSON for cfg_appSave()");
        cJSON_Delete(root);
        return;
    }

    FILE *file = file_create(dir, fileName);
    if (!file)
    {
        logs_log(LOG_ERROR, "Failed to create config file '%s' in '%s'", fileName, dir);
        free(json_str);
        cJSON_Delete(root);
        return;
    }

    fputs(json_str, file);
    logs_log(LOG_DEBUG, "Saved '%s' to '%s/%s'", APP_CONFIG_NAME, fullDir, fileName);

    file_close(file, APP_CONFIG_NAME);

    free(json_str);
    cJSON_Delete(root);
}

void cfg_appLoad(AppConfig_t *cfg, const char *dir, const char *fileName)
{
    if (!cfg)
    {
        logs_log(LOG_ERROR, "AppConfig_t pointer was NULL in cfg_appLoad()");
        return;
    }

    char fullDir[MAX_DIR_PATH_LENGTH];
    if (!file_dirExists(dir, fullDir))
    {
        logs_log(LOG_WARN, "Directory '%s' not found at '%s'. Using defaults.", dir, fullDir);
        return;
    }

    // Build final file path (../dir/fileName)
    char fullPath[MAX_DIR_PATH_LENGTH];
    snprintf(fullPath, MAX_DIR_PATH_LENGTH, "%s/%s", fullDir, fileName);
    logs_log(LOG_DEBUG, "Attempting to load '%s' from '%s'", fileName, fullPath);

    cJSON *root = load_json_file(fullPath, APP_CONFIG_NAME);
    if (!root)
    {
        logs_log(LOG_ERROR, "Failed to read or parse '%s' JSON at '%s'. Using defaults.", fileName, fullPath);

        // Attempt to rename the bad file
        char backupPath[MAX_DIR_PATH_LENGTH];
        snprintf(backupPath, sizeof(backupPath), "%s.old", fullPath);

        int renameResult = rename(fullPath, backupPath);
        if (renameResult == 0)
        {
            logs_log(LOG_WARN, "Corrupted config '%s' renamed to '%s'", fullPath, backupPath);
        }
        else
        {
            logs_log(LOG_ERROR, "Failed to rename corrupted config '%s' to '%s' (error %d)", fullPath, backupPath, renameResult);
        }

        // Regenerate a fresh default config
        logs_log(LOG_DEBUG, "Regenerating new default config '%s'", fileName);
        cfg_appSave(cfg, dir, fileName);
        return;
    }

    cJSON *window = cJSON_GetObjectItemCaseSensitive(root, "window");
    if (cJSON_IsObject(window))
    {
        cJSON *w = cJSON_GetObjectItem(window, "width");
        cJSON *h = cJSON_GetObjectItem(window, "height");
        cJSON *fs = cJSON_GetObjectItem(window, "fullscreen");

        if (cJSON_IsNumber(w))
            cfg->windowWidth = w->valueint;
        if (cJSON_IsNumber(h))
            cfg->windowHeight = h->valueint;
        if (cJSON_IsBool(fs))
            cfg->windowFullscreen = cJSON_IsTrue(fs);
    }

    cJSON *mouse = cJSON_GetObjectItemCaseSensitive(root, "mouse");
    if (cJSON_IsObject(mouse))
    {
        cJSON *cR = cJSON_GetObjectItem(mouse, "resetCursorOnMenuExit");
        cJSON *cS = cJSON_GetObjectItem(mouse, "mouseSensitivity");

        if (cJSON_IsBool(cR))
            cfg->resetCursorOnMenuExit = cJSON_IsTrue(cR);
        if (cJSON_IsNumber(cS))
            cfg->mouseSensitivity = cmath_clampD(cS->valuedouble, 0.01, 2.0);
    }

    cJSON *renderer = cJSON_GetObjectItemCaseSensitive(root, "renderer");
    if (cJSON_IsObject(renderer))
    {
        cJSON *vsync = cJSON_GetObjectItem(renderer, "vsync");
        cJSON *aniso = cJSON_GetObjectItem(renderer, "anisotropy");
        cJSON *fov = cJSON_GetObjectItem(renderer, "fov");

        if (cJSON_IsBool(vsync))
            cfg->vsync = cJSON_IsTrue(vsync);
        if (cJSON_IsNumber(aniso))
            cfg->anisotropy = aniso->valueint;
        if (cJSON_IsNumber(fov))
            cfg->cameraFOV = (float)fov->valuedouble;
    }

    logs_log(LOG_DEBUG, "Loaded '%s' configuration from '%s'", APP_CONFIG_NAME, fullPath);
    cJSON_Delete(root);
}

AppConfig_t cfg_loadOrCreate(void)
{
    char fullDir[MAX_DIR_PATH_LENGTH];

    if (file_exists(cfgFolder, APP_CONFIG_NAME, fullDir))
    {
        logs_log(LOG_DEBUG, "Loading existing cfg");
        cfg_appLoad(&appConfig, cfgFolder, APP_CONFIG_NAME);
    }
    else
    {
        logs_log(LOG_WARN, "Unable to locate config file. Creating default.");
        cfg_appSave(&appConfig, cfgFolder, APP_CONFIG_NAME);
    }

    return appConfig;
}

void cfg_appDestroy(void)
{
    if (pCfg)
    {
        file_close(pCfg, APP_CONFIG_NAME);
        pCfg = NULL;
    }
}

Input_t cfg_inputInit(void)
{
    return cfg_keyBindingsLoadOrCreate();
}

AppConfig_t cfg_appInit(void)
{
    return cfg_loadOrCreate();
}

void cfg_init(void)
{
    char fullDir[MAX_DIR_PATH_LENGTH];
    file_dirCreate(cfgFolder, fullDir);
}