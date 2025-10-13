#include <vulkan/vulkan.h>
#include "core/config.h"
#include <stdbool.h>
#include "main.h"
#include "core/logs.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include "fileIO.h"
#include <string.h>

static FILE *pCfg;

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
    cJSON_AddBoolToObject(window, "fullscreen", cfg->windowFullscreen);

    cJSON *renderer = cJSON_AddObjectToObject(root, "renderer");
    cJSON_AddBoolToObject(renderer, "vsync", cfg->vsync);
    cJSON_AddNumberToObject(renderer, "anisotropy", cfg->anisotropy);

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
    logs_log(LOG_INFO, "Saved '%s' to '%s/%s'", APP_CONFIG_NAME, fullDir, fileName);

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
        logs_log(LOG_INFO, "Regenerating new default config '%s'", fileName);
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

    cJSON *renderer = cJSON_GetObjectItemCaseSensitive(root, "renderer");
    if (cJSON_IsObject(renderer))
    {
        cJSON *vsync = cJSON_GetObjectItem(renderer, "vsync");
        cJSON *aniso = cJSON_GetObjectItem(renderer, "anisotropy");
        if (cJSON_IsBool(vsync))
            cfg->vsync = cJSON_IsTrue(vsync);
        if (cJSON_IsNumber(aniso))
            cfg->anisotropy = aniso->valueint;
    }

    logs_log(LOG_INFO, "Loaded '%s' configuration from '%s'", APP_CONFIG_NAME, fullPath);
    cJSON_Delete(root);
}

AppConfig_t cfg_loadOrCreate(void)
{
    const char *cfgFolder = "config";
    char fullDir[MAX_DIR_PATH_LENGTH];

    FileIO_Result_t result = file_dirCreate(cfgFolder, fullDir);

    switch (result)
    {
    case FILE_IO_RESULT_DIR_ALREADY_EXISTS:
        logs_log(LOG_DEBUG, "Loading existing cfg");
        cfg_appLoad(&appConfig, cfgFolder, APP_CONFIG_NAME);
        break;
    default:
        logs_log(LOG_WARN, "Unable to locate config file. Creating default.");
        cfg_appSave(&appConfig, cfgFolder, APP_CONFIG_NAME);
        break;
    }

    return appConfig;
}

void cfg_destroy(void)
{
    if (pCfg)
    {
        file_close(pCfg, APP_CONFIG_NAME);
        pCfg = NULL;
    }
}

AppConfig_t cfg_init(void)
{
    return cfg_loadOrCreate();
}