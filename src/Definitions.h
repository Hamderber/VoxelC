#pragma once

#define PROGRAM_NAME "VoxelC"
// exe is in ./bin so you have to first go up a directory
#define RESOURCE_TEXTURE_PATH "../res/textures/"
#define PI_D 3.1415926535897931
#define PI_F 3.1415927F

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
    VkCullModeFlagBits cullModeMask;
    VkFrontFace vertexWindingDirection;
    // Do not allow this value to be changed at runtime. Will cause memory issues with the amount of semaphors and fences.
    uint32_t maxFramesInFlight;
    double fixedTimeStep; // ex: 1.0 / 50.0
    uint32_t maxPhysicsFrameDelay;
    bool vulkanValidation;
} Config_t;

typedef struct
{
    // Time since last frame
    double frameTimeDelta;
    // Actual last time (not delta)
    double frameTimeLast;
    double frameTimeTotal;
    double framesPerSecond;
    // Fixed-step physics
    double fixedTimeAccumulated;
} Time_t;

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
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
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    uint32_t renderpassAttachmentCount;
    VkRenderPass pRenderPass;
    uint32_t framebufferCount;
    VkFramebuffer *pFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer *pCommandBuffers;
    VkSemaphore *imageAcquiredSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
    VkFence *imagesInFlight;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDescriptorSetLayout descriptorSetLayout;
    VkBuffer *pUniformBuffers;
    VkDeviceMemory *pUniformBufferMemories;
    // Array of pointers that Vulkan uses to access uniform buffers and their memory
    void **pUniformBuffersMapped;
    uint32_t currentFrame;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet *pDescriptorSets;
    // Change these to arrays once more than one texture is loaded
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
} Renderer_t;

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
} State_t;

typedef struct
{
    float x, y;
} Vec2f_t;

typedef struct
{
    float x, y, z;
} Vec3f_t;

// A w of 0 means rotation and 1 means position
typedef struct
{
    float x, y, z, w;
} Vec4f_t;

typedef struct
{
    float qx, qy, qz, qw;
} Quaternion_t;

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
static const Vec3f_t VEC3_ONE = {1.0f, 1.0f, 1.0f};
static const Vec3f_t VEC3_NEG_ONE = {-1.0f, -1.0f, -1.0f};
static const Vec3f_t VEC3_ZERO = {0.0f, 0.0f, 0.0f};

// Quaternions
static const Quaternion_t QUAT_IDENTITY = {0.0F, 0.0F, 0.0F, 1.0F};

// Matricies
// https://www.c-jump.com/bcc/common/Talk3/Math/GLM/GLM.html
// 4x4 Matrix column-major (array index is for each column)
// Struct members are the same as the shader codes'
typedef struct
{
    Vec4f_t m[4];
} Mat4c_t;

// COLUMN MAJOR!!!!
static const Mat4c_t MAT4_IDENTITY = {
    .m = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F},
    },
};

// Rendering
// https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/
// Struct members are the same as the shader codes'
typedef struct
{
    // It is critical to make sure that the sizing is
    // alligned for use with vulkan and shaders. 16 per row.
    // So for values less than 16 do Ex: alignas(16) float foo;
    // Just have everything start with alignas for safety
    alignas(16) Mat4c_t model;
    alignas(16) Mat4c_t view;
    alignas(16) Mat4c_t projection;
} UniformBufferObject_t;