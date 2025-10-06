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

// Logging
typedef enum
{
    LOG_INFO,
    LOG_WARN
} LogLevel_t;

// Engine
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
    VkFrontFace vertexWindingDirection;
    // Do not allow this value to be changed at runtime. Will cause memory issues with the amount of semaphors and fences.
    uint32_t maxFramesInFlight;
} Config_t;

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    // This is always null right now so that Vulkan uses its own allocator
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
    uint32_t imageAcquiredIndex;
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
    uint32_t renderpassAttachmentCount;
    VkRenderPass pRenderPass;
    uint32_t framebufferCount;
    VkFramebuffer *pFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore *imageAcquiredSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDescriptorSetLayout descriptorSetLayout;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    uint32_t currentFrame;
} Renderer_t;

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
} State_t;

typedef struct
{
    float x, y;
} Vec2f_t;

typedef struct
{
    float x, y, z;
} Vec3f_t;

typedef struct
{
    float x, y, z, w;
} Vec4f_t;

// Rendering
typedef struct
{
    Vec2f_t position;
    Vec3f_t color;
} ShaderVertex_t;

// Directions
static const Vec3f_t RIGHT = {1.0f, 0.0f, 0.0f};
static const Vec3f_t LEFT = {-1.0f, 0.0f, 0.0f};
static const Vec3f_t UP = {0.0f, 1.0f, 0.0f};
static const Vec3f_t DOWN = {0.0f, -1.0f, 0.0f};
static const Vec3f_t FORWARD = {0.0f, 0.0f, 1.0f};
static const Vec3f_t BACK = {0.0f, 0.0f, -1.0f};

// Colors
static const Vec3f_t RED = {1.0f, 0.0f, 0.0f};
static const Vec3f_t GREEN = {0.0f, 1.0f, 0.0f};
static const Vec3f_t BLUE = {0.0f, 0.0f, 1.0f};
static const Vec3f_t BLACK = {0.0f, 0.0f, 0.0f};
static const Vec3f_t WHITE = {1.0f, 1.0f, 1.0f};
static const Vec3f_t YELLOW = {1.0f, 1.0f, 0.0f};
static const Vec3f_t CYAN = {0.0f, 1.0f, 1.0f};
static const Vec3f_t MAGENTA = {1.0f, 0.0f, 1.0f};
static const Vec3f_t GRAY = {0.5f, 0.5f, 0.5f};

// Axes
static const Vec3f_t X_AXIS = {1.0f, 0.0f, 0.0f};
static const Vec3f_t Y_AXIS = {0.0f, 1.0f, 0.0f};
static const Vec3f_t Z_AXIS = {0.0f, 0.0f, 1.0f};

// Diagonals
static const Vec3f_t ONE = {1.0f, 1.0f, 1.0f};
static const Vec3f_t NEG_ONE = {-1.0f, -1.0f, -1.0f};
static const Vec3f_t ZERO = {0.0f, 0.0f, 0.0f};

// Matricies
// https://www.c-jump.com/bcc/common/Talk3/Math/GLM/GLM.html
// 4x4 Matrix column-major (array index is for each column)
// Struct members are the same as the shader codes'
typedef struct
{
    Vec4f_t m[4];
} Mat4c_t;

// 4x4 Matrix row-major (array index is for each row)
// Struct members are the same as the shader codes'
typedef struct
{
    Vec4f_t m[4];
} Mat4r_t;

// Rendering
// https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/
// Struct members are the same as the shader codes'
typedef struct
{
    Mat4c_t model;
    Mat4c_t view;
    Mat4c_t projection;
} UniformBufferObject_t;