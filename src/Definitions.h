#pragma once

#define PROGRAM_NAME "VoxelC"

/// @brief If error is anything but zero, log that (in red). Include originating filepath,
/// function name, line number, and the passed error. Optional additional variable args.
/// Throw a signal abort to notify the debugger as well. Both vulkan and glfw "error code"
/// 0 means "success" / "not an error" which is convenient. Must be sure to explicitly cast
/// the ERROR to an int errorCode for proper handling. Because this is a macro, the preprocessor
/// would just copy-paste the actual function call itself into the if statement AND the fprintf
/// otherwise. So if a funciton errors, it would actually be called TWICE if just ERROR was used
/// in the if and the fprintf. It must be "macroErrorCode" instead of "errorCode" because functions that
/// the preprocessor will paste this into may already define an "errorCode" variable, which would
/// be overshadowed by this one.
#define LOG_IF_ERROR(ERROR, FORMAT, ...)                                                \
    {                                                                                   \
        int macroErrorCode = ERROR;                                                     \
        if (macroErrorCode)                                                             \
        {                                                                               \
            fprintf(stderr, "\n\033[31m%s -> %s -> %i -> Error(%i):\033[0m\n\t" FORMAT, \
                    __FILE__, __func__, __LINE__, macroErrorCode, ##__VA_ARGS__);       \
            raise(SIGABRT);                                                             \
        }                                                                               \
    }

typedef enum
{
    LOG_INFO,
    LOG_WARN
} LogLevel_t;

typedef enum
{
    SWAPCHAIN_BUFFERING_DEFAULT = 0,
    SWAPCHAIN_BUFFERING_SINGLE = 1,
    SWAPCHAIN_BUFFERING_DOUBLE = 2,
    SWAPCHAIN_BUFFERING_TRIPLE = 3,
    SWAPCHAIN_BUFFERING_QUADRUPLE = 4,
} SwapchainBuffering_t;

typedef enum
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_FRAGMENT = 1,
} ShaderStage_t;

typedef struct
{
    const char *pApplicationName;
    const char *pEngineName;
    const char *pWindowTitle;
    uint32_t vkAPIVersion;
    SwapchainBuffering_t swapchainBuffering;
    VkComponentMapping swapchainComponentMapping;
    int windowWidth;
    int windowHeight;
    bool windowResizable;
    bool windowFullscreen;
} Config_t;

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkAllocationCallbacks *pAllocator;
    /// @brief UINT32_MAX means no family assigned (set to max during creation)
    uint32_t queueFamily;
} Context_t;

typedef struct
{
    // Swapchain
    // https://www.youtube.com/watch?v=nSzQcyQTtRY
    VkSwapchainKHR handle;
    uint32_t imageCount;
    bool recreate;
    VkImage *pImages;
    VkImageView *pImageViews;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkExtent2D imageExtent;
} Swapchain_t;

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *pWindow;
    int frameBufferWidth;
    int frameBufferHeight;
} Window_t;

typedef struct
{
    VkPipeline pGraphicsPipeline;
    VkPipelineLayout pPipelineLayout;
} Renderer_t;

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
} State_t;
