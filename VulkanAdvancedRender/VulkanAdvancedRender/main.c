// main.c : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "common.h"

enum MY_CONSTANTS
{
    MAX_VULKAN_LAYER_COUNT = 64,
    MAX_VULKAN_GLOBAL_EXT_PROPS = 256,
    MAX_GPU_COUNT = 8,
    MAX_QUEUE_FAMILY_PROPERTY_COUNT = 8,
    MAX_FORMAT_COUNT = 32,
    MAX_PRESENT_MODE_COUNT = 8,
    MAX_SWAPCHAIN_IMAGE_COUNT = 16,

    WINDOW_WIDTH = 640,
    WINDOW_HEIGHT = 640,
    FRAME_LAG = 2,

    s_depth_format = VK_FORMAT_D32_SFLOAT,

    FLATTEN_PIPELINE_INDEX = 0,
    GRAIENT_PIPELINE_INDEX,
    GEOMETRY_SHADER_PIPELINE_INDEX,
    TEXTURE_PIPELINE_INDEX,
    MESH_SHADER_PIPELINE_INDEX,
    TOTAL_PIPELINE_INDEX_COUNT,

    COLOR_DESCRIPTOR_SET_INDEX = 0,
    TEXTURE_DESCRIPTOR_SET_INDEX,
    DESCRIPTOR_SET_INDEX_COUNT
};

typedef struct SwapchainImageResources
{
    VkImage image;
    VkImageView view;
    VkImage msaaImage;          // MSAA image for render target (framebuffer)
    VkImageView msaaView;       // MSAA image view for render target (framebuffer)
    VkCommandBuffer cmd_buf;
    VkCommandBuffer graphics_to_present_cmd_buf;
    VkFramebuffer framebuffer;
} SwapchainImageResources;

typedef struct FlattenVertexUniform
{
    float u_factor[2];
    float u_angle;
} FlattenVertexUniform;

static_assert(sizeof(FlattenVertexUniform) == 12U, "Invalid FlattenVertexUniform size");

static VkLayerProperties s_layerProperties[MAX_VULKAN_LAYER_COUNT];
static const char* s_layerNames[MAX_VULKAN_LAYER_COUNT];
static VkExtensionProperties s_instanceExtensions[MAX_VULKAN_LAYER_COUNT][MAX_VULKAN_GLOBAL_EXT_PROPS];
static uint32_t s_layerCount;
static uint32_t s_instanceExtensionCounts[MAX_VULKAN_LAYER_COUNT];

static VkPhysicalDevice s_currPhysicalDevice = VK_NULL_HANDLE;
static VkInstance s_instance = VK_NULL_HANDLE;
static VkDevice s_specDevice = VK_NULL_HANDLE;
static bool s_supportIncrementalPresent = false;
static uint32_t s_queueFamilyPropertyCount = 0;
static uint32_t s_specQueueFamilyIndex = 0;
static VkSurfaceKHR s_surface = VK_NULL_HANDLE;
static VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
static VkSurfaceFormatKHR s_surfaceFormat = { 0 };
static uint32_t s_graphicsQueueFamilyIndex = 0;
static uint32_t s_presentQueueFamilyIndex = 0;
static VkQueue s_graphicsQueue = VK_NULL_HANDLE;
static VkQueue s_presentQueue = VK_NULL_HANDLE;
static SwapchainImageResources s_swapchainImageResources[MAX_SWAPCHAIN_IMAGE_COUNT] = { 0 };
static uint32_t s_swapchainImageCount = 0;
static uint32_t s_render_width, s_render_height;
static VkFence s_presentFences[FRAME_LAG] = { VK_NULL_HANDLE };
static VkSemaphore s_imageAcquiredSemaphores[FRAME_LAG] = { VK_NULL_HANDLE };
static VkSemaphore s_drawCompleteSemaphores[FRAME_LAG] = { VK_NULL_HANDLE };
static VkSemaphore s_imageOwnershipSemaphores[FRAME_LAG] = { VK_NULL_HANDLE };
static VkCommandPool s_commandPool = VK_NULL_HANDLE;
static VkCommandPool s_presentCommandPool = VK_NULL_HANDLE;
static VkCommandBuffer s_commandBuffers[1] = { VK_NULL_HANDLE };
static VkQueryPool s_timestampQueryPool = VK_NULL_HANDLE;
static VkQueryPool s_occlusionQueryPool = VK_NULL_HANDLE;
static VkBuffer s_hostVertexAndUniformBuffer = VK_NULL_HANDLE;
static VkDeviceMemory s_hostVertexUniformMemory = VK_NULL_HANDLE;
static VkDeviceMemory s_msaaColorImageMemory = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_descSetLayout = VK_NULL_HANDLE;
static VkPipelineLayout s_pipelineLayout = VK_NULL_HANDLE;
static VkRenderPass s_render_pass = VK_NULL_HANDLE;
static VkPipelineCache s_pipelineCaches[TOTAL_PIPELINE_INDEX_COUNT] = { VK_NULL_HANDLE };
static VkPipeline s_pipelines[TOTAL_PIPELINE_INDEX_COUNT] = { VK_NULL_HANDLE };
static VkDescriptorPool s_descPool = VK_NULL_HANDLE;
static VkDescriptorSet s_descriptorSet = VK_NULL_HANDLE;
static VkBuffer s_vertexCoordsBuffer = VK_NULL_HANDLE;
static VkBuffer s_textureCoordsBuffer = VK_NULL_HANDLE;
static VkBuffer s_colorBuffer = VK_NULL_HANDLE;
static VkBuffer s_uniformBuffer = VK_NULL_HANDLE;
static VkBuffer s_hostUploadTextureBuffer = VK_NULL_HANDLE;
static VkImage s_textureImage = VK_NULL_HANDLE;
static VkImageView s_textureImageView = VK_NULL_HANDLE;
static VkSampler s_textureSampler = VK_NULL_HANDLE;
static VkDeviceMemory s_vertexMemory = VK_NULL_HANDLE;
static VkDeviceMemory s_uniformMemory = VK_NULL_HANDLE;
static VkDeviceMemory s_hostUploadTextureMemory = VK_NULL_HANDLE;
static VkDeviceMemory s_textureMemory = VK_NULL_HANDLE;

static PFN_vkCmdDrawMeshTasksEXT dyn_vkCmdDrawMeshTasksEXT = NULL;

static uint32_t s_maxTaskWorkGroupTotalCount = 0U;
static uint32_t s_maxTaskWorkGroupInvocations = 0U;
static uint32_t s_maxTaskPayloadSize = 0U;
static uint32_t s_maxTaskSharedMemorySize = 0U;
static uint32_t s_maxTaskPayloadAndSharedMemorySize = 0U;
static uint32_t s_maxMeshWorkGroupTotalCount = 0U;
static uint32_t s_maxMeshWorkGroupInvocations = 0U;
static uint32_t s_maxMeshSharedMemorySize = 0U;
static uint32_t s_maxMeshPayloadAndSharedMemorySize = 0U;
static uint32_t s_maxMeshOutputMemorySize = 0U;
static uint32_t s_maxMeshPayloadAndOutputMemorySize = 0U;
static uint32_t s_maxMeshOutputComponents = 0U;
static uint32_t s_maxMeshOutputVertices = 0U;
static uint32_t s_maxMeshOutputPrimitives = 0U;
static uint32_t s_maxMeshOutputLayers = 0U;
static uint32_t s_maxPreferredTaskWorkGroupInvocations = 0U;
static uint32_t s_maxPreferredMeshWorkGroupInvocations = 0U;
static bool s_supportFragmentShadingRate = false;

static bool s_isRenderPrepared = false;
static bool s_isRotating = true;
static float s_currRorationDegree = 0.0f;
static float s_gpuTimestampPeriod = 0.0f;
static double s_currGPUDuration = 0.0;
static uint64_t s_currOcclusionCount = 0;

static struct
{
    VkImage image;
    VkImageView image_view;
    VkImage msaaImage;
    VkImageView msaaView;
    VkDeviceMemory device_memory;
    VkDeviceMemory msaaDeviceMemory;
} s_depthResource;

static const char* const s_deviceTypes[] = {
    "Other",
    "Integrated GPU",
    "Discrete GPU",
    "Virtual GPU",
    "CPU"
};

static const float s_vertex_coords_data[4 * 4] = {
    // bottom left
    -0.2f, 0.2f, 0.0f, 1.0f,
    // bottom right
    0.2f, 0.2f, 0.0f, 1.0f,
    // top left
    -0.2f, -0.2f, 0.0f, 1.0f,
    // top right
    0.2f, -0.2f, 0.0f, 1.0f
};

static const float s_texture_coords_data[4 * 2] = {
    // bottom left
    0.0f, 0.0f,
    // bottom right
    1.0f, 0.0f,
    // top left
    0.0f, 1.0f,
    // top right
    1.0f, 1.0f
};

static const float s_vertex_color_data[4 * 4] = {
    // bottom left
    0.9f, 0.1f, 0.1f, 1.0f,
    // bottom right
    0.1f, 0.9f, 0.1f, 1.0f,
    // top left
    0.1f, 0.1f, 0.9f, 1.0f,
    // top right
    0.9f, 0.1f, 0.9f, 1.0f
};

static inline bool IsSeperatePresentQueue(void)
{
    return s_graphicsQueueFamilyIndex != s_presentQueueFamilyIndex;
}

static VkResult init_global_extension_properties(uint32_t layerIndex)
{
    uint32_t instance_extension_count;
    VkResult res;
    VkLayerProperties* currLayer = &s_layerProperties[layerIndex];
    char const* const layer_name = currLayer->layerName;
    s_layerNames[layerIndex] = layer_name;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res != VK_SUCCESS) {
            return res;
        }

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }
        if (instance_extension_count > MAX_VULKAN_GLOBAL_EXT_PROPS) {
            instance_extension_count = MAX_VULKAN_GLOBAL_EXT_PROPS;
        }

        s_instanceExtensionCounts[layerIndex] = instance_extension_count;
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, s_instanceExtensions[layerIndex]);
    } while (res == VK_INCOMPLETE);

    return res;
}

static VkResult init_global_layer_properties(void)
{
    uint32_t instance_layer_count;
    VkResult res;

    // It's possible, though very rare, 
    // that the number of instance layers could change.
    // For example, installing something could include new layers that
    // the loader would pick up between the initial query for the count and the request for VkLayerProperties.
    // The loader indicates that by returning a VK_INCOMPLETE status and will update the the count parameter.
    // The count parameter will be updated with the number of entries loaded into the data pointer -
    // in case the number of layers went down or is smaller than the size given.
    do
    {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res != VK_SUCCESS) {
            return res;
        }

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }
        instance_layer_count = min(instance_layer_count, MAX_VULKAN_LAYER_COUNT);

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, s_layerProperties);
    } while (res == VK_INCOMPLETE);

    // Now gather the extension list for each instance layer.
    s_layerCount = instance_layer_count;
    for (uint32_t i = 0; i < instance_layer_count; i++)
    {
        res = init_global_extension_properties(i);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "Query global extension properties error: %d\n", res);
            break;
        }
    }

    return res;
}

static bool InitializeVulkanInstance(const char *appName, const char *engineName)
{
    VkResult result = init_global_layer_properties();
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "init_global_layer_properties failed: %d\n", result);
        return false;
    }
    printf("Found %u layer(s)...\n", s_layerCount);

    // Check whether a validation layer exists
    for (uint32_t i = 0; i < s_layerCount; ++i)
    {
        if (strstr(s_layerNames[i], "validation") != NULL)
        {
            fprintf(stderr, "Contains %s!\n", s_layerNames[i]);
            break;
        }
    }

    // Query the API version
    uint32_t apiVersion = VK_API_VERSION_1_0;
    result = vkEnumerateInstanceVersion(&apiVersion);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateInstanceVersion failed: %d\n", result);
        return false;
    }
    printf("Current API version: %u.%u.%u\n", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

    // initialize the VkApplicationInfo structure
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = appName,
        .applicationVersion = 1,
        .pEngineName = engineName,
        .engineVersion = 1,
        .apiVersion = apiVersion
    };

    uint32_t extPropCount = 0;
    VkExtensionProperties extProps[MAX_VULKAN_GLOBAL_EXT_PROPS];
    result = vkEnumerateInstanceExtensionProperties(NULL, &extPropCount, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateInstanceExtensionProperties for count failed: %d\n", result);
        return false;
    }
    printf("Found %u instance extensions!\n", extPropCount);
    extPropCount = min(extPropCount, MAX_VULKAN_GLOBAL_EXT_PROPS);

    result = vkEnumerateInstanceExtensionProperties(NULL, &extPropCount, extProps);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateInstanceExtensionProperties for content failed: %d\n", result);
        return false;
    }

    uint32_t availExtensionCount = 0;
    const char* availExtensionNames[8];

    bool supportSurface = false;
    bool supportFurfaceWin32 = false;
    bool supportColorSpaceExt = false;

    for (uint32_t i = 0; i < extPropCount; ++i)
    {
        const char* const currExtName = extProps[i].extensionName;

        if (strcmp(currExtName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
        {
            supportSurface = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
        {
            supportFurfaceWin32 = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
        {
            supportColorSpaceExt = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
    }
    printf("Found %u required instance extensions!\n", availExtensionCount);

    const char* notStr = "is";
    if (!supportSurface) {
        notStr = "not";
    }
    printf("%s %s supported!\n", VK_KHR_SURFACE_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportFurfaceWin32) {
        notStr = "not";
    }
    printf("%s %s supported!\n", VK_KHR_WIN32_SURFACE_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportColorSpaceExt) {
        notStr = "not";
    }
    printf("%s %s supported!\n", VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, notStr);
    notStr = "is";

    // initialize the VkInstanceCreateInfo structure
    const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = availExtensionCount,
        .ppEnabledExtensionNames = availExtensionNames,
        .enabledLayerCount = 0, // s_layerCount,
        .ppEnabledLayerNames = s_layerNames
    };

    result = vkCreateInstance(&inst_info, NULL, &s_instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        fprintf(stderr, "cannot find a compatible Vulkan ICD!\n");
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", result);
    }

    return result == VK_SUCCESS;
}

static void ParseDepthStencilResolveModes(char dstBuffer[], size_t bufferSize, VkResolveModeFlags flags)
{
    if (flags == VK_RESOLVE_MODE_NONE)
    {
        strcpy_s(dstBuffer, bufferSize, "None");
        return;
    }

    bool hasItem = false;
    if ((flags & VK_RESOLVE_MODE_SAMPLE_ZERO_BIT) != 0)
    {
        strcat_s(dstBuffer, bufferSize, "Sample Zero");
        hasItem = true;
    }
    if ((flags & VK_RESOLVE_MODE_AVERAGE_BIT) != 0)
    {
        strcat_s(dstBuffer, bufferSize, hasItem ? " | Average" : "Average");
        hasItem = true;
    }
    if ((flags & VK_RESOLVE_MODE_MIN_BIT) != 0)
    {
        strcat_s(dstBuffer, bufferSize, hasItem ? " | Min" : "Min");
        hasItem = true;
    }
    if ((flags & VK_RESOLVE_MODE_MAX_BIT) != 0)
    {
        strcat_s(dstBuffer, bufferSize, hasItem ? " | Max" : "Max");
        hasItem = true;
    }
    if ((flags & VK_RESOLVE_MODE_EXTERNAL_FORMAT_DOWNSAMPLE_ANDROID) != 0)
    {
        strcat_s(dstBuffer, bufferSize, hasItem ? " | External Format Downsample Android" : "External Format Downsample Android");
        hasItem = true;
    }
}

// Return the queue family count
static bool InitializeVulkanDevice(VkQueueFlagBits queueFlag)
{
    VkPhysicalDevice physicalDevices[MAX_GPU_COUNT] = { VK_NULL_HANDLE };
    uint32_t gpu_count = 0;
    VkResult res = vkEnumeratePhysicalDevices(s_instance, &gpu_count, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d\n", res);
        return false;
    }
    gpu_count = min(gpu_count, MAX_GPU_COUNT);

    res = vkEnumeratePhysicalDevices(s_instance, &gpu_count, physicalDevices);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d\n", res);
        return false;
    }

    // TODO: The following code is used to choose the working device and may be not necessary to other projects...
    const bool isSingle = gpu_count == 1;
    printf("\nThis application has detected there %s %u Vulkan capable device%s installed: \n",
        isSingle ? "is" : "are",
        gpu_count,
        isSingle ? "" : "s");

    VkPhysicalDeviceProperties props = { 0 };
    for (uint32_t i = 0; i < gpu_count; i++)
    {
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
        printf("\n======== Device %u info ========\n", i);
        printf("Device name: %s\n", props.deviceName);
        printf("Device type: %s\n", s_deviceTypes[props.deviceType]);
        printf("Vulkan API version: %u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
        printf("Driver version: %08X\n", props.driverVersion);
    }
    puts("Please choose which device to use...");

#ifdef _WIN32
    char inputBuffer[8] = { '\0' };
    const char* input = gets_s(inputBuffer, sizeof(inputBuffer));
    if (input == NULL) {
        input = "0";
    }
    const uint32_t deviceIndex = atoi(input);
#else
    char* input = NULL;
    ssize_t initLen = 0;
    const ssize_t len = getline(&input, &initLen, stdin);
    input[len - 1] = '\0';
    errno = 0;
    const uint32_t deviceIndex = (uint32_t)strtoul(input, NULL, 10);
    if (errno != 0)
    {
        fprintf(stderr, "Input error: %d! Invalid integer input!!\n", errno);
        return false;
    }
#endif // WIN32

    if (deviceIndex >= gpu_count)
    {
        fprintf(stderr, "Your input (%u) exceeds the max number of available devices (%u)\n", deviceIndex, gpu_count);
        return false;
    }
    printf("You have chosen device[%u]...\n", deviceIndex);

    s_currPhysicalDevice = physicalDevices[deviceIndex];

    // Query Vulkan extensions the current selected physical device supports
    uint32_t extPropCount = 0U;
    res = vkEnumerateDeviceExtensionProperties(s_currPhysicalDevice, NULL, &extPropCount, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateDeviceExtensionProperties for count failed: %d\n", res);
        return false;
    }
    printf("The current selected physical device supports %u device extensions!\n", extPropCount);
    extPropCount = min(extPropCount, MAX_VULKAN_GLOBAL_EXT_PROPS);

    VkExtensionProperties extProps[MAX_VULKAN_GLOBAL_EXT_PROPS];
    res = vkEnumerateDeviceExtensionProperties(s_currPhysicalDevice, NULL, &extPropCount, extProps);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateDeviceExtensionProperties for content failed: %d\n", res);
        return false;
    }

    uint32_t availExtensionCount = 0;
    const char* availExtensionNames[16];

    bool supportSwapchain = false;
    bool supportScalarBlock = false;
    bool supportDriverProperties = false;
    bool supportSPIRV1_4 = false;
    bool supportMeshShader = false;
    bool supportDepthStencilResolve = false;
    bool supportCreateRenderPass2 = false;

    for (uint32_t i = 0; i < extPropCount; ++i)
    {
        const char* const currExtName = extProps[i].extensionName;

        if (strcmp(currExtName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            supportSwapchain = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME) == 0)
        {
            supportScalarBlock = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME) == 0)
        {
            s_supportIncrementalPresent = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME) == 0)
        {
            supportDriverProperties = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_SPIRV_1_4_EXTENSION_NAME) == 0)
        {
            supportSPIRV1_4 = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_EXT_MESH_SHADER_EXTENSION_NAME) == 0)
        {
            supportMeshShader = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME) == 0)
        {
            s_supportFragmentShadingRate = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME) == 0)
        {
            supportDepthStencilResolve = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
        if (strcmp(currExtName, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME) == 0)
        {
            supportCreateRenderPass2 = true;
            availExtensionNames[availExtensionCount++] = currExtName;
            continue;
        }
    }

    const char* notStr = "is";
    if (!supportSwapchain) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_SWAPCHAIN_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportScalarBlock) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!s_supportIncrementalPresent) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportDriverProperties) {
        notStr = "not";
    }
    printf("%s feature %s supported or not explicitly given!\n", VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportSPIRV1_4) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_SPIRV_1_4_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportMeshShader) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_EXT_MESH_SHADER_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!s_supportFragmentShadingRate) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportDepthStencilResolve) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, notStr);
    notStr = "is";

    if (!supportCreateRenderPass2) {
        notStr = "not";
    }
    printf("%s feature %s supported!\n", VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, notStr);
    notStr = "is";

    printf("Available required device extension count: %u\n\n", availExtensionCount);

    char strBuffer[256] = { '\0' };

    // Query detail driver info
    VkPhysicalDeviceDriverProperties driverProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        // link to subgroupSizeProps node
        .pNext = NULL
    };

    VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT,
        .pNext = &driverProps
    };

    VkPhysicalDeviceFragmentShadingRatePropertiesKHR fragmentShadingRateProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR,
        .pNext = &meshShaderProps
    };

    VkPhysicalDeviceDepthStencilResolvePropertiesKHR depthStencilResolveProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES_KHR,
        .pNext = &fragmentShadingRateProps
    };

    VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        // link to driverProps
        .pNext = &depthStencilResolveProperties
    };

    // Query all above properties
    vkGetPhysicalDeviceProperties2(s_currPhysicalDevice, &properties2);

    printf("Detail driver info: %s %s\n", driverProps.driverName, driverProps.driverInfo);

    if (supportMeshShader)
    {
        s_maxTaskWorkGroupTotalCount = meshShaderProps.maxTaskWorkGroupTotalCount;
        printf("In task shader, max total work group count: %u (%uK)\n", s_maxTaskWorkGroupTotalCount, s_maxTaskWorkGroupTotalCount / 1024U);
        s_maxTaskWorkGroupInvocations = meshShaderProps.maxTaskWorkGroupInvocations;
        printf("In task shader, max work group invocation count: %u\n", s_maxTaskWorkGroupInvocations);
        s_maxPreferredTaskWorkGroupInvocations = meshShaderProps.maxPreferredTaskWorkGroupInvocations;
        printf("In task shader, max work group preferred invocation count: %u\n", s_maxPreferredTaskWorkGroupInvocations);
        s_maxTaskPayloadSize = meshShaderProps.maxTaskPayloadSize;
        printf("In task shader, max payload size: %uKB\n", s_maxTaskPayloadSize / 1024U);
        s_maxTaskSharedMemorySize = meshShaderProps.maxTaskSharedMemorySize;
        printf("In task shader, max shared memory size: %uKB\n", s_maxTaskSharedMemorySize / 1024U);
        s_maxTaskPayloadAndSharedMemorySize = meshShaderProps.maxTaskPayloadAndSharedMemorySize;
        printf("In task shader, max payload and shared memory size: %uKB\n", s_maxTaskPayloadAndSharedMemorySize / 1024U);
        s_maxMeshWorkGroupTotalCount = meshShaderProps.maxMeshWorkGroupTotalCount;
        printf("In mesh shader, max total work group count: %u (%uK)\n", s_maxMeshWorkGroupTotalCount, s_maxMeshWorkGroupTotalCount / 1024U);
        s_maxMeshWorkGroupInvocations = meshShaderProps.maxMeshWorkGroupInvocations;
        printf("In mesh shader, max work group invocation count: %u\n", s_maxMeshWorkGroupInvocations);
        s_maxPreferredMeshWorkGroupInvocations = meshShaderProps.maxPreferredMeshWorkGroupInvocations;
        printf("In mesh shader, max work group preferred invocation count: %u\n", s_maxPreferredMeshWorkGroupInvocations);
        s_maxMeshSharedMemorySize = meshShaderProps.maxMeshSharedMemorySize;
        printf("In mesh shader, max shared memory size: %uKB\n", s_maxMeshSharedMemorySize / 1024U);
        s_maxMeshPayloadAndSharedMemorySize = meshShaderProps.maxMeshPayloadAndSharedMemorySize;
        printf("In mesh shader, max payload and shared memory size: %uKB\n", s_maxMeshPayloadAndSharedMemorySize / 1024U);
        s_maxMeshOutputMemorySize = meshShaderProps.maxMeshOutputMemorySize;
        printf("In mesh shader, max output memory size: %uKB\n", s_maxMeshOutputMemorySize / 1024U);
        s_maxMeshPayloadAndOutputMemorySize = meshShaderProps.maxMeshPayloadAndOutputMemorySize;
        printf("In mesh shader, max payload and output memory size: %uKB\n", s_maxMeshPayloadAndOutputMemorySize / 1024U);
        s_maxMeshOutputComponents = meshShaderProps.maxMeshOutputComponents;
        printf("In mesh shader, max output component count: %u\n", s_maxMeshOutputComponents);
        s_maxMeshOutputVertices = meshShaderProps.maxMeshOutputVertices;
        printf("In mesh shader, max output vertex count: %u\n", s_maxMeshOutputVertices);
        s_maxMeshOutputPrimitives = meshShaderProps.maxMeshOutputPrimitives;
        printf("In mesh shader, max output primitive count: %u\n", s_maxMeshOutputPrimitives);
        s_maxMeshOutputLayers = meshShaderProps.maxMeshOutputLayers;
        printf("In mesh shader, max output layer count: %u\n", s_maxMeshOutputLayers);
    }
    if (s_supportFragmentShadingRate)
    {
        printf("min fragment shading rate attachment texel size -- width: %u, height: %u\n", fragmentShadingRateProps.minFragmentShadingRateAttachmentTexelSize.width, fragmentShadingRateProps.minFragmentShadingRateAttachmentTexelSize.height);
        printf("max fragment shading rate attachment texel size -- width: %u, height: %u\n", fragmentShadingRateProps.maxFragmentShadingRateAttachmentTexelSize.width, fragmentShadingRateProps.maxFragmentShadingRateAttachmentTexelSize.height);
        printf("max fragment shading rate attachment texel size aspect ratio: %u\n", fragmentShadingRateProps.maxFragmentShadingRateAttachmentTexelSizeAspectRatio);
        printf("support layered shading rate attachments? %s\n", fragmentShadingRateProps.layeredShadingRateAttachments != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate non trivial combiner ops? %s\n", fragmentShadingRateProps.fragmentShadingRateNonTrivialCombinerOps != VK_FALSE ? "YES" : "NO");
        printf("max fragment size -- width: %u, height: %u\n", fragmentShadingRateProps.maxFragmentSize.width, fragmentShadingRateProps.maxFragmentSize.height);
        printf("max fragment size aspect ratio: %u\n", fragmentShadingRateProps.maxFragmentSizeAspectRatio);
        printf("max fragment shading rate coverage samples: %u\n", fragmentShadingRateProps.maxFragmentShadingRateCoverageSamples);
        printf("max fragment shading rate rasterization samples: %u\n", fragmentShadingRateProps.maxFragmentShadingRateRasterizationSamples);
        printf("support fragment shading rate with shader depth stencil writes? %s\n", fragmentShadingRateProps.fragmentShadingRateWithShaderDepthStencilWrites != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate with sample mask? %s\n", fragmentShadingRateProps.fragmentShadingRateWithSampleMask != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate with shader sample mask? %s\n", fragmentShadingRateProps.fragmentShadingRateWithShaderSampleMask != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate with conservative rasterization? %s\n", fragmentShadingRateProps.fragmentShadingRateWithConservativeRasterization != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate with fragment shader inter-lock? %s\n", fragmentShadingRateProps.fragmentShadingRateWithFragmentShaderInterlock != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate with custom sample locations? %s\n", fragmentShadingRateProps.maxFragmentShadingRateCoverageSamples != VK_FALSE ? "YES" : "NO");
        printf("support fragment shading rate strict multipy combiner? %s\n", fragmentShadingRateProps.fragmentShadingRateStrictMultiplyCombiner != VK_FALSE ? "YES" : "NO");
    }
    if (supportDepthStencilResolve)
    {
        ParseDepthStencilResolveModes(strBuffer, sizeof(strBuffer), depthStencilResolveProperties.supportedDepthResolveModes);
        printf("support depth resolve modes: %s\n", strBuffer);

        strBuffer[0] = '\0';
        ParseDepthStencilResolveModes(strBuffer, sizeof(strBuffer), depthStencilResolveProperties.supportedStencilResolveModes);
        printf("support stencil resolve modes: %s\n", strBuffer);

        printf("support independent resolve None: %s\n", depthStencilResolveProperties.independentResolveNone != VK_FALSE ? "YES" : "NO");
        printf("Support independent resolve: %s\n", depthStencilResolveProperties.independentResolve != VK_FALSE ? "YES" : "NO");
    }

#if USE_MSAA_SAMPLE_COUNT > 0
    if (!supportDepthStencilResolve) {
        fprintf(stderr, "WARNING: Current device does not support either VK_KHR_depth_stencil_resolve or VK_KHR_create_renderpass2!");
    }
#endif

    s_gpuTimestampPeriod = properties2.properties.limits.timestampPeriod;

    // ==== The following is query the specific extension features in the feature chaining form ====
    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
        .pNext = NULL
    };

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .pNext = &scalarBlockLayoutFeature
    };

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR,
        .pNext = &meshShaderFeature
    };

    // physical device feature 2
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        // link to subgroupSizeControlFeature node
        .pNext = &fragmentShadingRateFeature
    };

    // Query all above features
    vkGetPhysicalDeviceFeatures2(s_currPhysicalDevice, &features2);

    if (scalarBlockLayoutFeature.scalarBlockLayout == VK_FALSE) {
        printf("%s feature not supported!\n", VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
    }
    if (supportMeshShader)
    {
        printf("Current device supports task shader? %s\n", meshShaderFeature.taskShader == VK_FALSE ? "NO" : "YES");
        printf("Current device supports mesh shader? %s\n", meshShaderFeature.meshShader == VK_FALSE ? "NO" : "YES");
        printf("Current device supports multi-view mesh shader? %s\n", meshShaderFeature.multiviewMeshShader == VK_FALSE ? "NO" : "YES");
        printf("Current device supports primitive fragment shading rate mesh shader? %s\n", meshShaderFeature.primitiveFragmentShadingRateMeshShader == VK_FALSE ? "NO" : "YES");
        printf("Current device supports mesh shader queries? %s\n", meshShaderFeature.meshShaderQueries == VK_FALSE ? "NO" : "YES");
    }
    if (supportMeshShader && supportSPIRV1_4 && meshShaderFeature.taskShader != VK_FALSE && meshShaderFeature.meshShader != VK_FALSE) {
        dyn_vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetInstanceProcAddr(s_instance, "vkCmdDrawMeshTasksEXT");
    }

    if (s_supportFragmentShadingRate)
    {
        printf("Current device support pipeline fragment shading rate? %s\n", fragmentShadingRateFeature.pipelineFragmentShadingRate != VK_FALSE ? "YES" : "NO");
        printf("Current device support primitvie fragment shading rate? %s\n", fragmentShadingRateFeature.primitiveFragmentShadingRate != VK_FALSE ? "YES" : "NO");
        printf("Current device support attachment fragment shading rate? %s\n", fragmentShadingRateFeature.attachmentFragmentShadingRate != VK_FALSE ? "YES" : "NO");
    }

    const float queue_priorities[1] = { 0.0f };
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities
    };

    VkQueueFamilyProperties queueFamilyProperties[MAX_QUEUE_FAMILY_PROPERTY_COUNT];

    vkGetPhysicalDeviceQueueFamilyProperties(s_currPhysicalDevice, &s_queueFamilyPropertyCount, NULL);
    s_queueFamilyPropertyCount = min(s_queueFamilyPropertyCount, MAX_QUEUE_FAMILY_PROPERTY_COUNT);

    vkGetPhysicalDeviceQueueFamilyProperties(s_currPhysicalDevice, &s_queueFamilyPropertyCount, queueFamilyProperties);

    bool found = false;
    for (uint32_t i = 0; i < s_queueFamilyPropertyCount; i++)
    {
        if ((queueFamilyProperties[i].queueFlags & queueFlag) != 0 &&
            // Query whether the current queue supports presentation operations
            vkGetPhysicalDeviceWin32PresentationSupportKHR(s_currPhysicalDevice, i) == VK_TRUE)
        {
            queue_info.queueFamilyIndex = i;
            found = true;
            break;
        }
    }

    s_specQueueFamilyIndex = queue_info.queueFamilyIndex;

    // There are two ways to enable features:
    // (1) Set pNext to a VkPhysicalDeviceFeatures2 structure and set pEnabledFeatures to NULL;
    // (2) or set pNext to NULL and set pEnabledFeatures to a VkPhysicalDeviceFeatures structure.
    // Here uses the first way
    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = availExtensionCount,
        .ppEnabledExtensionNames = availExtensionNames,
        .pEnabledFeatures = NULL
    };

    res = vkCreateDevice(s_currPhysicalDevice, &device_info, NULL, &s_specDevice);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDevice failed: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateVulkanSurface(HINSTANCE hInstane, HWND hWnd)
{
    // Destroy the surface object if it has already existed.
    if (s_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(s_instance, s_surface, NULL);
    }

    // Create a WSI surface for the window:
    const VkWin32SurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = hInstane,
        .hwnd = hWnd
    };

    VkResult res = vkCreateWin32SurfaceKHR(s_instance, &createInfo, NULL, &s_surface);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateWin32SurfaceKHR failed: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateVulkanSwapchain(void)
{
    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 supportsPresents[MAX_QUEUE_FAMILY_PROPERTY_COUNT] = { VK_FALSE };
    VkResult res;
    for (uint32_t i = 0; i < s_queueFamilyPropertyCount; ++i)
    {
        res = vkGetPhysicalDeviceSurfaceSupportKHR(s_currPhysicalDevice, i, s_surface, &supportsPresents[i]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkGetPhysicalDeviceSurfaceSupportKHR failed @%u: %d\n", i, res);
            return false;
        }
    }

    VkQueueFamilyProperties queue_props[MAX_QUEUE_FAMILY_PROPERTY_COUNT] = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(s_currPhysicalDevice, &s_queueFamilyPropertyCount, queue_props);

    // Search for a graphics and a present queue in the array of queue families,
    // try to find one that supports both
    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < s_queueFamilyPropertyCount; ++i)
    {
        if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueFamilyIndex == UINT32_MAX) {
                graphicsQueueFamilyIndex = i;
            }
            if (supportsPresents[i] == VK_TRUE)
            {
                graphicsQueueFamilyIndex = i;
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    if (presentQueueFamilyIndex == UINT32_MAX)
    {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < s_queueFamilyPropertyCount; ++i)
        {
            if (supportsPresents[i] == VK_TRUE)
            {
                presentQueueFamilyIndex = i;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
    {
        fprintf(stderr, "Could not find both graphics and present queues!\n");
        return false;
    }

    s_graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    s_presentQueueFamilyIndex = presentQueueFamilyIndex;
    const bool isSeparatePresentQueue = graphicsQueueFamilyIndex != presentQueueFamilyIndex;

    vkGetDeviceQueue(s_specDevice, graphicsQueueFamilyIndex, 0, &s_graphicsQueue);

    if (!isSeparatePresentQueue) {
        s_presentQueue = s_graphicsQueue;
    }
    else {
        vkGetDeviceQueue(s_specDevice, presentQueueFamilyIndex, 0, &s_presentQueue);
    }

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities = { 0 };
    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_currPhysicalDevice, s_surface, &surfCapabilities);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %d\n", res);
        return false;
    }

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple buffering
    uint32_t desiredNumOfSwapchainImages = 3;
    desiredNumOfSwapchainImages = max(desiredNumOfSwapchainImages, surfCapabilities.minImageCount);
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
    if (surfCapabilities.maxImageCount > 0) {
        // Application must settle for fewer images than desired:
        desiredNumOfSwapchainImages = min(desiredNumOfSwapchainImages, surfCapabilities.maxImageCount);
    }

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount = 0;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(s_currPhysicalDevice, s_surface, &formatCount, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceSurfaceFormatsKHR for count failed: %d\n", res);
        return false;
    }
    formatCount = min(formatCount, MAX_FORMAT_COUNT);

    VkSurfaceFormatKHR surfaceFormats[MAX_FORMAT_COUNT];
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(s_currPhysicalDevice, s_surface, &formatCount, surfaceFormats);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceSurfaceFormatsKHR for content failed: %d\n", res);
        return false;
    }

    // Prefer extended SRGB, then normal SRGB and last UNORM RGBA
    const uint64_t preferredSurfaceFormatList[] = {
        ((uint64_t)VK_FORMAT_R16G16B16A16_SFLOAT << 32) | VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
        ((uint64_t)VK_FORMAT_A2B10G10R10_UNORM_PACK32 << 32) | VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        ((uint64_t)VK_FORMAT_R8G8B8A8_SRGB << 32) | VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        ((uint64_t)VK_FORMAT_B8G8R8A8_SRGB << 32) | VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        ((uint64_t)VK_FORMAT_R8G8B8A8_UNORM << 32) | VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        ((uint64_t)VK_FORMAT_B8G8R8A8_UNORM << 32)| VK_COLORSPACE_SRGB_NONLINEAR_KHR
    };
    enum {
        preferredFormatCount = sizeof(preferredSurfaceFormatList) / sizeof(preferredSurfaceFormatList[0])
    };

    bool foundPreferredFormat = false;
    for (int i = 0; i < preferredFormatCount; ++i)
    {
        const uint64_t currPreferredFormat = preferredSurfaceFormatList[i];
        for (uint32_t j = 0; j < formatCount; ++j)
        {
            const VkFormat format = surfaceFormats[j].format;
            const VkColorSpaceKHR colorSpace = surfaceFormats[j].colorSpace;
            if (currPreferredFormat == (((uint64_t)format << 32) | colorSpace))
            {
                s_surfaceFormat = surfaceFormats[j];
                foundPreferredFormat = true;
                break;
            }
        }
        if (foundPreferredFormat) {
            break;
        }
    }
    if (!foundPreferredFormat)
    {
        fprintf(stderr, "Can't find our preferred formats... Falling back to first exposed format. Rendering may be incorrect.\n");
        s_surfaceFormat = surfaceFormats[0];
    }

    const char* currSurfaceFormatStr = "Unknown";
    switch (s_surfaceFormat.format)
    {
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        currSurfaceFormatStr = "VK_FORMAT_R16G16B16A16_SFLOAT";
        break;

    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        currSurfaceFormatStr = "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        break;

    case VK_FORMAT_R8G8B8A8_SRGB:
        currSurfaceFormatStr = "VK_FORMAT_R8G8B8A8_SRGB";
        break;

    case VK_FORMAT_B8G8R8A8_SRGB:
        currSurfaceFormatStr = "VK_FORMAT_B8G8R8A8_SRGB";
        break;

    case VK_FORMAT_R8G8B8A8_UNORM:
        currSurfaceFormatStr = "VK_FORMAT_R8G8B8A8_UNORM";
        break;

    case VK_FORMAT_B8G8R8A8_UNORM:
        currSurfaceFormatStr = "VK_FORMAT_B8G8R8A8_UNORM";
        break;

    default:
        break;
    }

    printf("Current found preferred surface format: %s\n", currSurfaceFormatStr);

    VkImageFormatProperties imageFormatProperties = { 0 };
    res = vkGetPhysicalDeviceImageFormatProperties(s_currPhysicalDevice, s_surfaceFormat.format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT, &imageFormatProperties);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceImageFormatProperties failed: %d\n", res);
        return false;
    }

    printf("Current surface format properties: maxExtent: %ux%ux%u, maxMipLevels: %u, maxArrayLayers: %u, sampleCounts: 0x%02X, maxResourceSize: %zu\n",
        imageFormatProperties.maxExtent.width, imageFormatProperties.maxExtent.height, imageFormatProperties.maxExtent.depth, imageFormatProperties.maxMipLevels,
        imageFormatProperties.maxArrayLayers, imageFormatProperties.sampleCounts, imageFormatProperties.maxResourceSize);

    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCapabilities.currentExtent.width == 0xffffffffU || surfCapabilities.currentExtent.height == 0xffffffffU)
    {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.
        swapchainExtent.width = s_render_width;
        swapchainExtent.height = s_render_height;

        swapchainExtent.width = max(swapchainExtent.width, surfCapabilities.minImageExtent.width);
        swapchainExtent.width = min(swapchainExtent.width, surfCapabilities.maxImageExtent.width);

        swapchainExtent.height = max(swapchainExtent.height, surfCapabilities.minImageExtent.height);
        swapchainExtent.height = min(swapchainExtent.height, surfCapabilities.maxImageExtent.height);
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
        s_render_width = surfCapabilities.currentExtent.width;
        s_render_height = surfCapabilities.currentExtent.height;
    }

    if (s_render_width == 0 || s_render_height == 0) {
        // In the window minimized situation
        return true;
    }

    uint32_t presentModeCount = 0;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(s_currPhysicalDevice, s_surface, &presentModeCount, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceSurfacePresentModesKHR for count failed: %d\n", res);
        return false;
    }
    presentModeCount = min(presentModeCount, MAX_PRESENT_MODE_COUNT);

    VkPresentModeKHR presentModes[MAX_PRESENT_MODE_COUNT] = { 0 };
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(s_currPhysicalDevice, s_surface, &presentModeCount, presentModes);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetPhysicalDeviceSurfacePresentModesKHR for content failed: %d\n", res);
        return false;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    // and to have no tearing.  It's a great default present mode to use.
    const VkPresentModeKHR preferredPresentModes[] = {
        //VK_PRESENT_MODE_FIFO_RELAXED_KHR,
        VK_PRESENT_MODE_FIFO_KHR
    };

    //  There are times when you may wish to use another present mode.  The
    //  following code shows how to select them, and the comments provide some
    //  reasons you may wish to use them.
    //
    // It should be noted that Vulkan 1.0 doesn't provide a method for
    // synchronizing rendering with the presentation engine's display.  There
    // is a method provided for throttling rendering with the display, but
    // there are some presentation engines for which this method will not work.
    // If an application doesn't throttle its rendering, and if it renders much
    // faster than the refresh rate of the display, this can waste power on
    // mobile devices.  That is because power is being spent rendering images
    // that may never be seen.

    // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
    // tearing, or have some way of synchronizing their rendering with the
    // display.
    // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
    // generally render a new presentable image every refresh cycle, but are
    // occasionally early.  In this case, the application wants the new image
    // to be displayed instead of the previously-queued-for-presentation image
    // that has not yet been displayed.
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
    // render a new presentable image every refresh cycle, but are occasionally
    // late.  In this case (perhaps because of stuttering/latency concerns),
    // the application wants the late image to be immediately displayed, even
    // though that may mean some tearing.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

    bool foundPreferredPresentMode = false;
    for (size_t i = 0; i < sizeof(preferredPresentModes) / sizeof(preferredPresentModes[0]); ++i)
    {
        const VkPresentModeKHR currPreferredPresentMode = preferredPresentModes[i];
        for (uint32_t j = 0; j < presentModeCount; ++j)
        {
            if (presentModes[j] == currPreferredPresentMode)
            {
                swapchainPresentMode = currPreferredPresentMode;
                foundPreferredPresentMode = true;
                break;
            }
        }
        if (foundPreferredPresentMode) {
            break;
        }
    }

    if (!foundPreferredPresentMode)
    {
        puts("Preferred present mode is not found!");
        return false;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if ((surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        preTransform = surfCapabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (size_t i = 0; i < sizeof(compositeAlphaFlags) / sizeof(compositeAlphaFlags[0]); ++i)
    {
        if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainKHR oldSwapchain = s_swapchain;

    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = s_surface,
        .minImageCount = desiredNumOfSwapchainImages,
        .imageFormat = s_surfaceFormat.format,
        .imageColorSpace = s_surfaceFormat.colorSpace,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_specQueueFamilyIndex,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = swapchainPresentMode,
        .clipped = true,
        .oldSwapchain = oldSwapchain
    };
    res = vkCreateSwapchainKHR(s_specDevice, &swapchainCreateInfo, NULL, &s_swapchain);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateSwapchainKHR failed: %d\n", res);
        return false;
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(s_specDevice, oldSwapchain, NULL);
    }

    res = vkGetSwapchainImagesKHR(s_specDevice, s_swapchain, &s_swapchainImageCount, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetSwapchainImagesKHR for count failed: %d\n", res);
        return false;
    }

    VkImage swapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
    res = vkGetSwapchainImagesKHR(s_specDevice, s_swapchain, &s_swapchainImageCount, swapchainImages);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkGetSwapchainImagesKHR for content faile: %d\n", res);
        return false;
    }

#if USE_MSAA_SAMPLE_COUNT > 0
    const VkImageCreateInfo msaaImageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = s_surfaceFormat.format,
        .extent = { swapchainExtent.width, swapchainExtent.height, 1U },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = (VkSampleCountFlagBits)USE_MSAA_SAMPLE_COUNT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = (uint32_t[]){ graphicsQueueFamilyIndex },
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    res = vkCreateImage(s_specDevice, &msaaImageCreateInfo, NULL, &s_swapchainImageResources[0].msaaImage);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateImage for texture faild: %d\n", res);
        return false;
    }

    VkPhysicalDeviceMemoryProperties memoryProperties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(s_currPhysicalDevice, &memoryProperties);

    VkMemoryRequirements memoryRequirements = { 0 };
    vkGetImageMemoryRequirements(s_specDevice, s_swapchainImageResources[0].msaaImage, &memoryRequirements);
    const VkDeviceSize alignmentMask = memoryRequirements.alignment - 1U;
    const VkDeviceSize msaaImageBufferSize = (memoryRequirements.size + alignmentMask) & ~alignmentMask;
    memoryRequirements.size = msaaImageBufferSize * s_swapchainImageCount;

    // Find host visible property memory type index
    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((memoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            // We prefer using this bit to back an image with VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT usage.
            (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= memoryRequirements.size) {
            // found our memory type!
            break;
        }
    }
    if (memoryTypeIndex == memoryProperties.memoryTypeCount)
    {
        // Not found preferred memory type!
        // Try the secondary memory type...
        for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
        {
            if ((memoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
                continue;
            }
            const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
            if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
                memoryProperties.memoryHeaps[memoryType.heapIndex].size >= memoryRequirements.size) {
                // found our memory type!
                break;
            }
        }
    }

    const VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &memAllocInfo, NULL, &s_msaaColorImageMemory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for texture failed: %d\n", res);
        return false;
    }

    VkImageViewCreateInfo msaaImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = VK_NULL_HANDLE,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = s_surfaceFormat.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U
        }
    };
#endif // USE_MSAA

    for (uint32_t i = 0; i < s_swapchainImageCount; i++)
    {
        const VkImageViewCreateInfo swapchainImageView = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = s_surfaceFormat.format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, 
                .baseArrayLayer = 0, .layerCount = 1}
        };
        res = vkCreateImageView(s_specDevice, &swapchainImageView, NULL, &s_swapchainImageResources[i].view);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateImageView for swapchain failed: %d\n", res);
            return false;
        }

        s_swapchainImageResources[i].image = swapchainImages[i];

#if USE_MSAA_SAMPLE_COUNT > 0
        if (i > 0U)
        {
            res = vkCreateImage(s_specDevice, &msaaImageCreateInfo, NULL, &s_swapchainImageResources[i].msaaImage);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkCreateImage for MSAA image faild: %d\n", res);
                return false;
            }
        }

        res = vkBindImageMemory(s_specDevice, s_swapchainImageResources[i].msaaImage, s_msaaColorImageMemory, i * msaaImageBufferSize);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkBindImageMemory for MSAA image failed: %d\n", res);
            break;
        }

        msaaImageViewCreateInfo.image = s_swapchainImageResources[i].msaaImage;
        res = vkCreateImageView(s_specDevice, &msaaImageViewCreateInfo, NULL, &s_swapchainImageResources[i].msaaView);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateImageView for MSAA image view failed: %d\n", res);
            break;
        }
#endif
    }

    return true;
}

static bool CreateFencesAndSemaphores(void)
{
    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    const VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < FRAME_LAG; i++)
    {
        VkResult res = vkCreateFence(s_specDevice, &fenceCreateInfo, NULL, &s_presentFences[i]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateFence @%u failed: %d\n", i, res);
            return false;
        }

        res = vkCreateSemaphore(s_specDevice, &semaphoreCreateInfo, NULL, &s_imageAcquiredSemaphores[i]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateSemaphore for s_imageAcquiredSemaphores @%u failed: %d\n", i, res);
            return false;
        }

        res = vkCreateSemaphore(s_specDevice, &semaphoreCreateInfo, NULL, &s_drawCompleteSemaphores[i]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateSemaphore for s_drawCompleteSemaphores @%u failed: %d\n", i, res);
            return false;
        }

        if (IsSeperatePresentQueue())
        {
            res = vkCreateSemaphore(s_specDevice, &semaphoreCreateInfo, NULL, &s_imageOwnershipSemaphores[i]);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkCreateSemaphore for s_imageOwnershipSemaphores @%u failed: %d\n", i, res);
                return false;
            }
        }
    }

    return true;
}

static bool BuildPresentImageOwnershipTransition(uint32_t imageIndex)
{
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL,
    };
    VkResult res = vkBeginCommandBuffer(s_swapchainImageResources[imageIndex].graphics_to_present_cmd_buf, &cmd_buf_info);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBeginCommandBuffer for present command @%u failed: %d\n", imageIndex, res);
        return false;
    }

    VkImageMemoryBarrier image_ownership_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                    .pNext = NULL,
                                                    .srcAccessMask = 0,
                                                    .dstAccessMask = 0,
                                                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                    .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
                                                    .dstQueueFamilyIndex = s_presentQueueFamilyIndex,
                                                    .image = s_swapchainImageResources[imageIndex].image,
                                                    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };
    // Transfer the swapchain image ownership from graphics queue family to present queue family
    vkCmdPipelineBarrier(s_swapchainImageResources[imageIndex].graphics_to_present_cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_ownership_barrier);

    res = vkEndCommandBuffer(s_swapchainImageResources[imageIndex].graphics_to_present_cmd_buf);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEndCommandBuffer failed in BuildPresentImageCommand: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateCommandBufferAndBeginCommand(void)
{
    const VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = s_graphicsQueueFamilyIndex
    };

    VkResult res = vkCreateCommandPool(s_specDevice, &cmdPoolInfo, NULL, &s_commandPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateCommandPool failed: %d\n", res);
        return false;
    }

    // Create the command buffer from the command pool
    const VkCommandBufferAllocateInfo cmdBufAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = s_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)(sizeof(s_commandBuffers) / sizeof(s_commandBuffers[0]))
    };

    res = vkAllocateCommandBuffers(s_specDevice, &cmdBufAllocInfo, s_commandBuffers);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateCommandBuffers failed: %d\n", res);
        return false;
    }

    // Create command buffers for swapchain images
    for (uint32_t i = 0; i < s_swapchainImageCount; i++)
    {
        res = vkAllocateCommandBuffers(s_specDevice, &cmdBufAllocInfo, &s_swapchainImageResources[i].cmd_buf);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkAllocateCommandBuffers for swapchain @%u failed: %d\n", i, res);
            return false;
        }
    }

    if (IsSeperatePresentQueue())
    {
        const VkCommandPoolCreateInfo present_cmd_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = s_presentQueueFamilyIndex,
            .flags = 0,
        };
        res = vkCreateCommandPool(s_specDevice, &present_cmd_pool_info, NULL, &s_presentCommandPool);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateCommandPool failed: %d\n", res);
            return false;
        }
        const VkCommandBufferAllocateInfo present_cmd_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = s_presentCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        for (uint32_t i = 0; i < s_swapchainImageCount; i++)
        {
            res = vkAllocateCommandBuffers(s_specDevice, &present_cmd_info, &s_swapchainImageResources[i].graphics_to_present_cmd_buf);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkAllocateCommandBuffers for graphics to present failed: %d\n", res);
                return false;
            }

            if (!BuildPresentImageOwnershipTransition(i)) {
                return false;
            }
        }
    }

    const VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    res = vkBeginCommandBuffer(s_commandBuffers[0], &cmdBufBeginInfo);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBeginCommandBuffer failed: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateQueryPools(void)
{
    // Create the timestamp query pool
    const VkQueryPoolCreateInfo timestampQueryPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = 2 * s_swapchainImageCount,
        .pipelineStatistics = 0
    };

    VkResult res = vkCreateQueryPool(s_specDevice, &timestampQueryPoolCreateInfo, NULL, &s_timestampQueryPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateQueryPool for timestamp failed: %d\n", res);
        return false;
    }

    const VkQueryPoolCreateInfo occlusionQueryPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_OCCLUSION,
        .queryCount = s_swapchainImageCount,
        .pipelineStatistics = 0
    };
    
    res = vkCreateQueryPool(s_specDevice, &occlusionQueryPoolCreateInfo, NULL, &s_occlusionQueryPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateQueryPool for occlusion failed: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateVertexAndUniformBuffersAndMemories(void)
{    
    VkPhysicalDeviceMemoryProperties memoryProperties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(s_currPhysicalDevice, &memoryProperties);

    const VkBufferCreateInfo hostVertexBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(s_vertex_coords_data) + sizeof(s_texture_coords_data) + sizeof(s_vertex_color_data) + sizeof(FlattenVertexUniform),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex
    };
    VkResult res = vkCreateBuffer(s_specDevice, &hostVertexBufferCreateInfo, NULL, &s_hostVertexAndUniformBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for host vertex and uniform buffer failed: %d\n", res);
        return false;
    }

    VkMemoryRequirements hostVertexMemoryRequirements = { 0 };
    vkGetBufferMemoryRequirements(s_specDevice, s_hostVertexAndUniformBuffer, &hostVertexMemoryRequirements);

    uint32_t memoryTypeIndex;
    // Find host visible property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((hostVertexMemoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 &&
            (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= hostVertexMemoryRequirements.size)
        {
            // found our memory type!
            printf("Host visible memory size: %zuMB\n", memoryProperties.memoryHeaps[memoryType.heapIndex].size / (1024 * 1024));
            break;
        }
    }

    const VkMemoryAllocateInfo hostMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = hostVertexMemoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &hostMemAllocInfo, NULL, &s_hostVertexUniformMemory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for host vertex and uniform memory failed: %d\n", res);
        return res;
    }

    res = vkBindBufferMemory(s_specDevice, s_hostVertexAndUniformBuffer, s_hostVertexUniformMemory, 0);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
        return res;
    }

    const VkBufferCreateInfo deviceCoordsBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(s_vertex_coords_data),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex
    };

    const VkBufferCreateInfo deviceTexCoordsBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(s_texture_coords_data),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex
    };

    const VkBufferCreateInfo deviceColorBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(s_vertex_color_data),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex
    };

    const VkBufferCreateInfo uniformBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(FlattenVertexUniform),
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex
    };

    res = vkCreateBuffer(s_specDevice, &deviceCoordsBufferCreateInfo, NULL, &s_vertexCoordsBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for vertex coords buffer failed: %d\n", res);
        return false;
    }

    res = vkCreateBuffer(s_specDevice, &deviceTexCoordsBufferCreateInfo, NULL, &s_textureCoordsBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for texture coords buffer failed: %d\n", res);
        return false;
    }

    res = vkCreateBuffer(s_specDevice, &deviceColorBufferCreateInfo, NULL, &s_colorBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for color buffer failed: %d\n", res);
        return false;
    }

    VkMemoryRequirements deviceVertexMemoryRequirements = { 0 };
    vkGetBufferMemoryRequirements(s_specDevice, s_vertexCoordsBuffer, &deviceVertexMemoryRequirements);
    const VkDeviceSize alignedSizeMask = deviceVertexMemoryRequirements.alignment - 1U;
    const VkDeviceSize vertexBufferMemorySize = (deviceVertexMemoryRequirements.size + alignedSizeMask) & ~alignedSizeMask;
    const VkDeviceSize totalDeviceVertexBufferSize = vertexBufferMemorySize * 3;

    // Find host visible property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((deviceVertexMemoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= totalDeviceVertexBufferSize) {
            // found our memory type!
            break;
        }
    }

    const VkMemoryAllocateInfo deviceVertexMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = totalDeviceVertexBufferSize,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &deviceVertexMemAllocInfo, NULL, &s_vertexMemory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for vertex buffer failed: %d\n", res);
        return false;
    }

    res = vkBindBufferMemory(s_specDevice, s_vertexCoordsBuffer, s_vertexMemory, 0U);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory for vertex coords buffer failed: %d\n", res);
        return false;
    }

    res = vkBindBufferMemory(s_specDevice, s_textureCoordsBuffer, s_vertexMemory, vertexBufferMemorySize);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory for texture coords buffer failed: %d\n", res);
        return false;
    }

    res = vkBindBufferMemory(s_specDevice, s_colorBuffer, s_vertexMemory, vertexBufferMemorySize * 2U);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory for color buffer failed: %d\n", res);
        return false;
    }

    res = vkCreateBuffer(s_specDevice, &uniformBufferCreateInfo, NULL, &s_uniformBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for uniform buffer failed: %d\n", res);
        return false;
    }

    vkGetBufferMemoryRequirements(s_specDevice, s_uniformBuffer, &deviceVertexMemoryRequirements);

    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((deviceVertexMemoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= deviceVertexMemoryRequirements.size) {
            // found our memory type!
            break;
        }
    }

    const VkMemoryAllocateInfo deviceUniformMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = deviceVertexMemoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &deviceUniformMemAllocInfo, NULL, &s_uniformMemory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for uniform buffer failed: %d\n", res);
        return false;
    }

    res = vkBindBufferMemory(s_specDevice, s_uniformBuffer, s_uniformMemory, 0);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory for coords buffer failed: %d\n", res);
        return false;
    }

    // Fill the vertex coordinate data into the host memory object
    uint8_t *hostData = NULL;
    res = vkMapMemory(s_specDevice, s_hostVertexUniformMemory, 0, hostVertexMemoryRequirements.size, 0, &hostData);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkMapMemory in CreateVertexAndUniformBuffersAndMemories failed: %d\n", res);
        return false;
    }

    memcpy(hostData, s_vertex_coords_data, sizeof(s_vertex_coords_data));
    memcpy(&hostData[sizeof(s_vertex_coords_data)], s_texture_coords_data, sizeof(s_texture_coords_data));
    memcpy(&hostData[sizeof(s_vertex_coords_data) + sizeof(s_texture_coords_data)], s_vertex_color_data, sizeof(s_vertex_color_data));

    vkUnmapMemory(s_specDevice, s_hostVertexUniformMemory);

    return true;
}

static void CopyFromHostToDeviceBuffersAndSync(void)
{
    const VkBufferCopy copyVertexCoordsRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sizeof(s_vertex_coords_data)
    };
    const VkBufferCopy copyTextureCoordsRegion = {
        .srcOffset = sizeof(s_vertex_coords_data),
        .dstOffset = 0,
        .size = sizeof(s_texture_coords_data)
    };
    const VkBufferCopy copyColorRegion = {
        .srcOffset = sizeof(s_vertex_coords_data) + sizeof(s_texture_coords_data),
        .dstOffset = 0,
        .size = sizeof(s_vertex_color_data)
    };

    vkCmdCopyBuffer(s_commandBuffers[0], s_hostVertexAndUniformBuffer, s_vertexCoordsBuffer, 1, &copyVertexCoordsRegion);
    vkCmdCopyBuffer(s_commandBuffers[0], s_hostVertexAndUniformBuffer, s_textureCoordsBuffer, 1, &copyTextureCoordsRegion);
    vkCmdCopyBuffer(s_commandBuffers[0], s_hostVertexAndUniformBuffer, s_colorBuffer, 1, &copyColorRegion);

    VkBufferMemoryBarrier bufferBarriers[] = {
        // vertex coords buffer barrier
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .dstQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .buffer = s_vertexCoordsBuffer,
            .offset = 0,
            .size = sizeof(s_vertex_coords_data)
        },
        // texture coords buffer barrier
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .dstQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .buffer = s_textureCoordsBuffer,
            .offset = 0,
            .size = sizeof(s_texture_coords_data)
        },
        // color buffer barrier
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .dstQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .buffer = s_colorBuffer,
            .offset = 0,
            .size = sizeof(s_vertex_color_data)
        }
    };

    vkCmdPipelineBarrier(s_commandBuffers[0], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                        0, NULL, (uint32_t)(sizeof(bufferBarriers) / sizeof(bufferBarriers[0])), bufferBarriers, 0, NULL);
}

static bool CreateDepthReource(void)
{
    VkPhysicalDeviceMemoryProperties memoryProperties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(s_currPhysicalDevice, &memoryProperties);

    const VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = s_depth_format,
        .extent = { s_render_width, s_render_height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult res = vkCreateImage(s_specDevice, &imageCreateInfo, NULL, &s_depthResource.image);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateImage for depth faild: %d\n", res);
        return false;
    }

    VkMemoryRequirements memoryRequirements = { 0 };
    vkGetImageMemoryRequirements(s_specDevice, s_depthResource.image, &memoryRequirements);

    uint32_t memoryTypeIndex;
    // Find host visible property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((memoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= memoryRequirements.size)
        {
            // found our memory type!
            break;
        }
    }

    const VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &memAllocInfo, NULL, &s_depthResource.device_memory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for depth failed: %d\n", res);
        return false;
    }

    res = vkBindImageMemory(s_specDevice, s_depthResource.image, s_depthResource.device_memory, 0);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindImageMemory for depth failed: %d\n", res);
        return false;
    }

    const VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = s_depthResource.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = s_depth_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, 
            .baseMipLevel = 0, 
            .levelCount = 1, 
            .baseArrayLayer = 0, 
            .layerCount = 1
        }
    };

    res = vkCreateImageView(s_specDevice, &imageViewCreateInfo, NULL, &s_depthResource.image_view);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateImageView for depth failed: %d\n", res);
        return false;
    }

#if USE_MSAA_SAMPLE_COUNT > 0
    const VkImageCreateInfo msaaImageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = s_depth_format,
            .extent = { s_render_width, s_render_height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = (VkSampleCountFlagBits)USE_MSAA_SAMPLE_COUNT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &s_graphicsQueueFamilyIndex,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    res = vkCreateImage(s_specDevice, &msaaImageCreateInfo, NULL, &s_depthResource.msaaImage);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateImage for MSAA depth faild: %d\n", res);
        return false;
    }

    vkGetImageMemoryRequirements(s_specDevice, s_depthResource.msaaImage, &memoryRequirements);

    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
    {
        if ((memoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            // We prefer using this bit to back an image with VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT usage.
            (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0 &&
            memoryProperties.memoryHeaps[memoryType.heapIndex].size >= memoryRequirements.size)
        {
            // found our memory type!
            break;
        }
    }
    if (memoryTypeIndex == memoryProperties.memoryTypeCount)
    {
        // Not found preferred memory type!
        // Try the secondary memory type...
        for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
        {
            if ((memoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
                continue;
            }
            const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
            if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
                memoryProperties.memoryHeaps[memoryType.heapIndex].size >= memoryRequirements.size)
            {
                // found our memory type!
                break;
            }
        }
    }

    const VkMemoryAllocateInfo msaaMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(s_specDevice, &msaaMemAllocInfo, NULL, &s_depthResource.msaaDeviceMemory);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for MSAA depth failed: %d\n", res);
        return false;
    }

    res = vkBindImageMemory(s_specDevice, s_depthResource.msaaImage, s_depthResource.msaaDeviceMemory, 0U);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindImageMemory for depth failed: %d\n", res);
        return false;
    }

    const VkImageViewCreateInfo msaaImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = s_depthResource.msaaImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = s_depth_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    res = vkCreateImageView(s_specDevice, &msaaImageViewCreateInfo, NULL, &s_depthResource.msaaView);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateImageView for MSAA depth failed: %d\n", res);
        return false;
    }
#endif

    return true;
}

static bool CreateDescriptorSetAndPipelineLayout(void)
{
    const VkDescriptorSetLayoutBinding layoutBindings[] = {
        // uniform transfer buffer
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_MESH_BIT_EXT,    // The `bind = 0` uniform buffer will be used both in vertex shader and mesh shader. 
            .pImmutableSamplers = NULL,
        },
        // sampler
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL
        }
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = (uint32_t)(sizeof(layoutBindings) / sizeof(layoutBindings[0])),
        .pBindings = layoutBindings,
    };

    VkResult res = vkCreateDescriptorSetLayout(s_specDevice, &descriptor_layout, NULL, &s_descSetLayout);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDescriptorSetLayout failed: %d\n", res);
        return false;
    }

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = &s_descSetLayout,
    };

    res = vkCreatePipelineLayout(s_specDevice, &pPipelineLayoutCreateInfo, NULL, &s_pipelineLayout);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreatePipelineLayout failed: %d\n", res);
        return false;
    }

    return true;
}

static bool CreateRenderPass(void)
{
    // The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
    // because at the start of the renderpass, we don't care about their contents.
    // At the start of the subpass, the color attachment's layout will be transitioned
    // to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
    // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
    // the renderpass, the color attachment's layout will be transitioned to
    // LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
    // the renderpass, no barriers are necessary.
#if USE_MSAA_SAMPLE_COUNT > 0
    const VkAttachmentDescription2KHR attachments[] = {
        // MSAA color attachment
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,
            .pNext = NULL,
            .flags = 0,
            .format = s_surfaceFormat.format,
            .samples = (VkSampleCountFlagBits)USE_MSAA_SAMPLE_COUNT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        // MSAA depth attachment
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,
            .pNext = NULL,
            .flags = 0,
            .format = s_depth_format,
            .flags = 0,
            .samples = (VkSampleCountFlagBits)USE_MSAA_SAMPLE_COUNT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
        // color resolved attachment
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,
            .pNext = NULL,
            .flags = 0,
            .flags = 0,
            .format = s_surfaceFormat.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        // depth resolved attachment
        {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,
            .pNext = NULL,
            .flags = 0,
            .flags = 0,
            .format = s_depth_format,
            .flags = 0,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    const VkAttachmentReference2 color_reference = {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,
        .pNext = NULL,
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference2 depth_reference = {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,
        .pNext = NULL,
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference2 color_resolved_reference = {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,
        .pNext = NULL,
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference2 depth_resolved_reference = {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,
        .pNext = NULL,
        .attachment = 3,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescriptionDepthStencilResolveKHR depthStencilResolve = {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR,
        .pNext = NULL,
        .depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT_KHR,
        .stencilResolveMode = VK_RESOLVE_MODE_NONE,
        .pDepthStencilResolveAttachment = &depth_resolved_reference
    };

    const VkSubpassDescription2KHR subpasses[1] = {
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,
            .pNext = &depthStencilResolve,
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_reference,
            .pResolveAttachments = &color_resolved_reference,
            .pDepthStencilAttachment = &depth_reference,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL,
        }
    };

    VkSubpassDependency2KHR attachmentDependencies[] = {
        // Depth buffer is shared between swapchain images
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR,
            .pNext = NULL,
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
            .viewOffset = 0
        },
        // Image Layout Transition
        {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR,
            .pNext = NULL,
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .dependencyFlags = 0,
            .viewOffset = 0
        }
    };

    const VkRenderPassCreateInfo2KHR renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = (uint32_t)(sizeof(attachments) / sizeof(attachments[0])),
        .pAttachments = attachments,
        .subpassCount = (uint32_t)(sizeof(subpasses) / sizeof(subpasses[0])),
        .pSubpasses = subpasses,
        .dependencyCount = (uint32_t)(sizeof(attachmentDependencies) / sizeof(attachmentDependencies[0])),
        .pDependencies = attachmentDependencies,
        .correlatedViewMaskCount = 0U,
        .pCorrelatedViewMasks = NULL
    };

    VkResult res = vkCreateRenderPass2(s_specDevice, &renderPassCreateInfo, NULL, &s_render_pass);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateRenderPass failed: %d\n", res);
        return false;
    }

#else
    const VkAttachmentDescription attachments[] = {
        // color attachment
        {
            .flags = 0,
            .format = s_surfaceFormat.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        // depth attachment
        {
            .format = s_depth_format,
            .flags = 0,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    const VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpasses[1] = {
        {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_reference,
            .pResolveAttachments = NULL,
            .pDepthStencilAttachment = &depth_reference,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = NULL
        }
    };

    VkSubpassDependency attachmentDependencies[] = {
        // Depth buffer is shared between swapchain images
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0
        },
        // Image Layout Transition
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .dependencyFlags = 0
        },
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = (uint32_t)(sizeof(attachments) / sizeof(attachments[0])),
        .pAttachments = attachments,
        .subpassCount = (uint32_t)(sizeof(subpasses) / sizeof(subpasses[0])),
        .pSubpasses = subpasses,
        .dependencyCount = (uint32_t)(sizeof(attachmentDependencies) / sizeof(attachmentDependencies[0])),
        .pDependencies = attachmentDependencies
    };

    VkResult res = vkCreateRenderPass(s_specDevice, &renderPassCreateInfo, NULL, &s_render_pass);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateRenderPass failed: %d\n", res);
        return false;
    }
#endif

    return true;
}

bool CreateShaderModule(const char* fileName, VkShaderModule* pShaderModule)
{
    FILE* fp = GeneralOpenFile(fileName);
    if (fp == NULL) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint32_t* codeBuffer = malloc(fileLen);
    if (codeBuffer != NULL) {
        fread(codeBuffer, 1, fileLen, fp);
    }
    fclose(fp);

    const VkShaderModuleCreateInfo moduleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = fileLen,
        .pCode = codeBuffer
    };

    VkResult res = vkCreateShaderModule(s_specDevice, &moduleCreateInfo, NULL, pShaderModule);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateShaderModule failed: %d\n", res);
    }

    free(codeBuffer);

    return res == VK_SUCCESS;
}

static bool CreateGraphicsPipeline(const char* vertSPVFilePath, const char* fragSPVFilePath, int index)
{
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    do
    {
        if (!CreateShaderModule(vertSPVFilePath, &vertexShaderModule)) break;
        if (!CreateShaderModule(fragSPVFilePath, &fragmentShaderModule)) break;

        // two shader stages
        const VkPipelineShaderStageCreateInfo shaderStages[] = {
            // vertex shader
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertexShaderModule,
                .pName = "main",
                .pSpecializationInfo = NULL
            },
            // fragment shader
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragmentShaderModule,
                .pName = "main",
                .pSpecializationInfo = NULL
            }
        };

        const VkVertexInputBindingDescription vertexInputBindings[] = {
            // vertex coords buffer
            {
                .binding = VERTEX_BUFFER_LOCATION_INDEX,
                .stride = sizeof(float[4]),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            // color buffer
            {
                .binding = COLOR_BUFFER_LOCATION_INDEX,
                .stride = sizeof(float[4]),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };

        const VkVertexInputAttributeDescription vertexInputAttributes[] = {
            // inPos attribute
            {
                .location = VERTEX_BUFFER_LOCATION_INDEX,
                .binding = VERTEX_BUFFER_LOCATION_INDEX,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = 0
            },
            // inColor attribute
            {
                .location = COLOR_BUFFER_LOCATION_INDEX,
                .binding = COLOR_BUFFER_LOCATION_INDEX,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = 0
            }
        };

        const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .vertexBindingDescriptionCount = (uint32_t)(sizeof(vertexInputBindings) / sizeof(vertexInputBindings[0])),
            .pVertexBindingDescriptions = vertexInputBindings,
            .vertexAttributeDescriptionCount = (uint32_t)(sizeof(vertexInputAttributes) / sizeof(vertexInputAttributes[0])),
            .pVertexAttributeDescriptions = vertexInputAttributes
        };

        const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        };

        const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = NULL,     // As the viewport state is dynamic, this member is ignored.
            .scissorCount = 1,
            .pScissors = NULL       // As the scissor state is dynamic, this member is ignored.
        };

        const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            // Index 0 and 1 squares rotate around the y-axis and x-axis separately so that the back face should not be culled.
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 1.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f,
        };

        const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .rasterizationSamples = USE_MSAA_SAMPLE_COUNT > 0 ? (VkSampleCountFlagBits)(USE_MSAA_SAMPLE_COUNT) : VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0.0f,
            .pSampleMask = NULL,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
        };

        const VkStencilOpState stencilOpState = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .compareMask = 0,
            .writeMask = 0,
            .reference = 0
        };

        const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = stencilOpState,
            .back = stencilOpState,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 0.0f
        };

        const VkPipelineColorBlendAttachmentState attatchmentStates[1] = {
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = 0x0fU
            }
        };

        const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_CLEAR,
            .attachmentCount = (uint32_t)(sizeof(attatchmentStates) / sizeof(attatchmentStates[0])),
            .pAttachments = attatchmentStates,
            .blendConstants = { 0.0f }
        };

        const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .dynamicStateCount = 2U,
            .pDynamicStates = (VkDynamicState[]) { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }
        };

        const VkPipelineFragmentShadingRateStateCreateInfoKHR fragmentShadingRateStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR,
            .pNext = NULL,
            .fragmentSize = {
                .width = USE_MSAA_SAMPLE_COUNT > 0 ? 2U : 1U,
                .height = USE_MSAA_SAMPLE_COUNT > 0 ? 2U : 4U
            },
            .combinerOps = { [0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR, [1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR }
        };

        const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = (index == 0 && s_supportFragmentShadingRate) ? &fragmentShadingRateStateCreateInfo : NULL,
            .stageCount = (uint32_t)(sizeof(shaderStages) / sizeof(shaderStages[0])),
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputStateCreateInfo,
            .pInputAssemblyState = &inputAssemblyStateCreateInfo,
            .pTessellationState = NULL,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &rasterizationStateCreateInfo,
            .pMultisampleState = &multisampleStateCreateInfo,
            .pDepthStencilState = &depthStencilStateCreateInfo,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = &dynamicStateCreateInfo,
            .layout = s_pipelineLayout,
            .renderPass = s_render_pass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0
        };

        const VkPipelineCacheCreateInfo pipelineCacheInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .initialDataSize = 0,
            .pInitialData = NULL
        };

        res = vkCreatePipelineCache(s_specDevice, &pipelineCacheInfo, NULL, &s_pipelineCaches[index]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreatePipelineCache failed: %d\n", res);
            break;
        }

        res = vkCreateGraphicsPipelines(s_specDevice, s_pipelineCaches[index], 1, &pipelineCreateInfo, NULL, &s_pipelines[index]);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateGraphicsPipelines failed: %d\n", res);
            break;
        }
    }
    while (false);

    if (vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(s_specDevice, vertexShaderModule, NULL);
    }
    if (fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(s_specDevice, fragmentShaderModule, NULL);
    }

    return res == VK_SUCCESS;
}

static bool CreateDescriptorPoolAndSet(void)
{
    const VkDescriptorPoolSize poolSizes[] = {
        // uniform translate buffer
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1U,
        },
        // sampler combined with image
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1U
        }
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = 2U,
        .poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(poolSizes[0])),
        .pPoolSizes = poolSizes,
    };

    VkResult res = vkCreateDescriptorPool(s_specDevice, &descriptor_pool, NULL, &s_descPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDescriptorPool failed: %d\n", res);
        return false;
    }

    const VkDescriptorSetAllocateInfo alloc_info = { 
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = s_descPool,
        .descriptorSetCount = 1U,
        .pSetLayouts = &s_descSetLayout
    };

    const VkDescriptorBufferInfo buffer_info = {
        .buffer = s_uniformBuffer,
        .offset = 0,
        .range = sizeof(FlattenVertexUniform)
    };

    const VkDescriptorImageInfo image_info = {
        .sampler = s_textureSampler,
        .imageView = s_textureImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL     // This image layout is specified as the one before drawcall and after transfer.
    };

    // There's no need to free `descriptor_set`, since VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag is not set
    // in the member `flags` in `VkDescriptorPoolCreateInfo` object.
    res = vkAllocateDescriptorSets(s_specDevice, &alloc_info, &s_descriptorSet);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateDescriptorSets failed: %d\n", res);
        return false;
    }

    const VkWriteDescriptorSet writes[] = {
        // uniform translate buffer
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = s_descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = NULL
        },
        // sampler combined with image
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = s_descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info,
            .pBufferInfo = NULL,
            .pTexelBufferView = NULL
        }
    };

    vkUpdateDescriptorSets(s_specDevice, (uint32_t)(sizeof(writes) / sizeof(writes[0])), writes, 0, NULL);

    return true;
}

static bool CreateFramebuffers(void)
{
#if USE_MSAA_SAMPLE_COUNT > 0
    // This `attachments` MUST BE coherent with the one in renderpass creation.
    VkImageView attachments[] = { VK_NULL_HANDLE, s_depthResource.msaaView, VK_NULL_HANDLE, s_depthResource.image_view };
#else
    // This `attachments` MUST BE coherent with the one in renderpass creation.
    VkImageView attachments[] = { VK_NULL_HANDLE, s_depthResource.image_view };
#endif

    const VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = s_render_pass,
        .attachmentCount = (uint32_t)(sizeof(attachments) / sizeof(attachments[0])),
        .pAttachments = attachments,
        .width = s_render_width,
        .height = s_render_height,
        .layers = 1
    };

    for (uint32_t i = 0; i < s_swapchainImageCount; ++i)
    {
#if USE_MSAA_SAMPLE_COUNT > 0
        attachments[0] = s_swapchainImageResources[i].msaaView;
        attachments[2] = s_swapchainImageResources[i].view;
#else
        attachments[0] = s_swapchainImageResources[i].view;
#endif
        VkResult res = vkCreateFramebuffer(s_specDevice, &framebufferCreateInfo, NULL, &s_swapchainImageResources[i].framebuffer);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateFramebuffer @%u failed: %d\n", i, res);
            return false;
        }
    }

    return true;
}

static bool RecordCommandsForDraw(VkCommandBuffer inputCmdBuf, uint32_t swapchainIndex)
{
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL,
    };
    VkResult res = vkBeginCommandBuffer(inputCmdBuf, &cmd_buf_info);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBeginCommandBuffer in RecordCommandsForDraw @%u failed: %d\n", swapchainIndex, res);
        return false;
    }

    // Reset the query pools
    vkCmdResetQueryPool(inputCmdBuf, s_occlusionQueryPool, swapchainIndex, 1);
    vkCmdResetQueryPool(inputCmdBuf, s_timestampQueryPool, swapchainIndex * 2, 1);

    // Begin the timestamp query
    vkCmdWriteTimestamp(inputCmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, s_timestampQueryPool, swapchainIndex * 2);

    // This `clearValues` MUST BE coherent with the attachments in renderpass creation.
    const VkClearValue clearValues[] = {
        { .color.float32 = { 0.4f, 0.5f, 0.4f, 1.0f } },
        { .depthStencil = { .depth = 1.0f, .stencil = 0 } }
#if USE_MSAA_SAMPLE_COUNT > 0
        //,
        // attachment[2], i.e. color resolve is VK_ATTACHMENT_LOAD_OP_DONT_CARE
        //{.color.float32 = { 0.4f, 0.5f, 0.4f, 1.0f } },
        // attachment[3], i.e. depth resolve is VK_ATTACHMENT_LOAD_OP_DONT_CARE
        //{.depthStencil = {.depth = 1.0f, .stencil = 0 } }
#endif
    };
    const VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = s_render_pass,
        .framebuffer = s_swapchainImageResources[swapchainIndex].framebuffer,
        .renderArea = {
            .offset = { .x = 0, .y = 0 },
            .extent = {.width = s_render_width, .height = s_render_height }
        },
        .clearValueCount = (uint32_t)(sizeof(clearValues) / sizeof(clearValues[0])),
        .pClearValues = clearValues,
    };

    // ==== The following code block is in the render pass instance. ====
#if USE_MSAA_SAMPLE_COUNT > 0
    const VkSubpassBeginInfoKHR subpassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO_KHR,
        .pNext = NULL,
        .contents = VK_SUBPASS_CONTENTS_INLINE
    };
    vkCmdBeginRenderPass2(inputCmdBuf, &renderPassBeginInfo, &subpassBeginInfo);
#else
    vkCmdBeginRenderPass(inputCmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
#endif

    const VkBuffer vertexBuffers[] = {
        s_vertexCoordsBuffer,       // VERTEX_BUFFER_LOCATION_INDEX
        s_colorBuffer,              // COLOR_BUFFER_LOCATION_INDEX
        s_textureCoordsBuffer       // TEXCOORDS_BUFFER_LOCATION_INDEX
    };
    const VkDeviceSize vertexoffsets[] = { 0U, 0U, 0U };
    vkCmdBindVertexBuffers(inputCmdBuf, 0, sizeof(vertexBuffers) / sizeof(vertexBuffers[0]), vertexBuffers, vertexoffsets);

    vkCmdBindDescriptorSets(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelineLayout, 0, 1U,
                            &s_descriptorSet, 0, NULL);

    const bool isWidthShorterThanHeight = s_render_width < s_render_height;
    const VkViewport viewport = {
        .x = isWidthShorterThanHeight ? 0.0f : (s_render_width - s_render_height) / 2.0f,
        .y = isWidthShorterThanHeight ? (s_render_height - s_render_width) / 2.0f : 0.0f,
        .width = isWidthShorterThanHeight ? (float)s_render_width : (float)s_render_height,
        .height = isWidthShorterThanHeight ? (float)s_render_width : (float)s_render_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(inputCmdBuf, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = { .width = s_render_width, .height = s_render_height }
    };
    vkCmdSetScissor(inputCmdBuf, 0, 1, &scissor);

    // Begin the occlusion query
    vkCmdBeginQuery(inputCmdBuf, s_occlusionQueryPool, swapchainIndex, VK_QUERY_CONTROL_PRECISE_BIT);

    // Draw
    vkCmdBindPipeline(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelines[FLATTEN_PIPELINE_INDEX]);
    vkCmdDraw(inputCmdBuf, 4, 1, 0, 0);

    vkCmdBindPipeline(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelines[GRAIENT_PIPELINE_INDEX]);
    vkCmdDraw(inputCmdBuf, 4, 1, 0, 0);

    vkCmdBindPipeline(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelines[TEXTURE_PIPELINE_INDEX]);
    vkCmdDraw(inputCmdBuf, 4, 1, 0, 0);

    // Draw the geometry shader test primitives
    vkCmdBindPipeline(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelines[GEOMETRY_SHADER_PIPELINE_INDEX]);
    vkCmdDraw(inputCmdBuf, 1, 1, 0, 0);

    if (s_pipelines[MESH_SHADER_PIPELINE_INDEX] != VK_NULL_HANDLE && dyn_vkCmdDrawMeshTasksEXT != NULL)
    {
        // Dispatch task shader
        vkCmdBindPipeline(inputCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipelines[MESH_SHADER_PIPELINE_INDEX]);
        dyn_vkCmdDrawMeshTasksEXT(inputCmdBuf, 1U, 1U, 1U);
    }

    // End the occlusion query
    vkCmdEndQuery(inputCmdBuf, s_occlusionQueryPool, swapchainIndex);

    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
#if USE_MSAA_SAMPLE_COUNT > 0
    const VkSubpassEndInfoKHR subpassEndInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO_KHR,
        .pNext = NULL
    };
    vkCmdEndRenderPass2(inputCmdBuf, &subpassEndInfo);
#else
    vkCmdEndRenderPass(inputCmdBuf);
#endif

    if (IsSeperatePresentQueue())
    {
        // We have to transfer ownership from the graphics queue family to the
        // present queue family to be able to present.  Note that we don't have
        // to transfer from present queue family back to graphics queue family at
        // the start of the next frame because we don't care about the image's
        // contents at that point.
        const VkImageMemoryBarrier image_ownership_barrier = { 
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
            .dstQueueFamilyIndex = s_presentQueueFamilyIndex,
            .image = s_swapchainImageResources[swapchainIndex].image,
            .subresourceRange = { 
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
                .baseMipLevel = 0, 
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(inputCmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
            NULL, 1, &image_ownership_barrier);
    }

    // Update uniform buffer data on device side.
    // ATTENTION: vkCmdCopyBuffer MUST BE only called outside of a render pass instance!
    const size_t srcOffset = sizeof(s_vertex_coords_data) + sizeof(s_texture_coords_data) + sizeof(s_vertex_color_data);
    const VkBufferCopy copyUniformRegion = {
        .srcOffset = srcOffset,
        .dstOffset = 0,
        .size = sizeof(FlattenVertexUniform)
    };
    vkCmdCopyBuffer(inputCmdBuf, s_hostVertexAndUniformBuffer, s_uniformBuffer, 1, &copyUniformRegion);

    const VkBufferMemoryBarrier copyBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = s_graphicsQueueFamilyIndex,
        .dstQueueFamilyIndex = s_graphicsQueueFamilyIndex,
        .buffer = s_uniformBuffer,
        .offset = 0,
        .size = sizeof(FlattenVertexUniform)
    };
    vkCmdPipelineBarrier(inputCmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                        0, NULL, 1, &copyBarrier, 0, NULL);

    // End the query timestamp
    vkCmdResetQueryPool(inputCmdBuf, s_timestampQueryPool, swapchainIndex * 2 + 1, 1);
    vkCmdWriteTimestamp(inputCmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_timestampQueryPool, swapchainIndex * 2 + 1);

    res = vkEndCommandBuffer(inputCmdBuf);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEndCommandBuffer for RecordCommandsForDraw failed: %d\n", res);
        return false;
    }

    return true;
}

static bool FlushInitCommand(void)
{
    // This function could get called twice if the texture uses a staging buffer
    // In that case the second call should be ignored
    if (s_commandBuffers[0] == VK_NULL_HANDLE) return true;

    VkResult res = vkEndCommandBuffer(s_commandBuffers[0]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEndCommandBuffer for init command buffer failed: %d\n", res);
        return false;
    }

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = (uint32_t)(sizeof(s_commandBuffers) / sizeof(s_commandBuffers[0])),
        .pCommandBuffers = s_commandBuffers,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL
    };

    VkFence submitFence = VK_NULL_HANDLE;
    const VkFenceCreateInfo submitFenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0 };
    res = vkCreateFence(s_specDevice, &submitFenceCreateInfo, NULL, &submitFence);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateFence for submitFence failed: %d\n", res);
        return false;
    }

    do
    {
        res = vkQueueSubmit(s_graphicsQueue, 1, &submit_info, submitFence);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkQueueSubmit in init command buffer failed: %d\n", res);
            break;
        }

        res = vkWaitForFences(s_specDevice, 1, &submitFence, VK_TRUE, UINT64_MAX);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkWaitForFences in init command buffer failed: %d\n", res);
            break;
        }
    }
    while (false);

    vkDestroyFence(s_specDevice, submitFence, NULL);
    
    // This command buffer is one shot for the init flush submission,
    // and will NOT be used any more.
    vkFreeCommandBuffers(s_specDevice, s_commandPool, (uint32_t)(sizeof(s_commandBuffers) / sizeof(s_commandBuffers[0])), s_commandBuffers);
    s_commandBuffers[0] = VK_NULL_HANDLE;

    if (s_hostUploadTextureBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(s_specDevice, s_hostUploadTextureBuffer, NULL);
        s_hostUploadTextureBuffer = VK_NULL_HANDLE;
    }
    if (s_hostUploadTextureMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(s_specDevice, s_hostUploadTextureMemory, NULL);
        s_hostUploadTextureMemory = VK_NULL_HANDLE;
    }

    return res == VK_SUCCESS;
}

static bool UpdateUniformData(int currImageIndex)
{
    if (!s_isRotating) return true;

    const size_t srcOffset = sizeof(s_vertex_coords_data) + sizeof(s_texture_coords_data) + sizeof(s_vertex_color_data);

    FlattenVertexUniform* hostUniformData = NULL;
    VkResult res = vkMapMemory(s_specDevice, s_hostVertexUniformMemory, srcOffset, sizeof(FlattenVertexUniform), 0, &hostUniformData);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkMapMemory for update uniform data failed: %d\n", res);
        return false;
    }

    hostUniformData->u_factor[0] = 1.0f;
    hostUniformData->u_factor[1] = 1.0f;
    hostUniformData->u_angle = s_currRorationDegree;

    s_currRorationDegree += 1.0f;
    if (s_currRorationDegree >= 360.0f) {
        s_currRorationDegree = 0.0f;
    }

    vkUnmapMemory(s_specDevice, s_hostVertexUniformMemory);

    return true;
}

static size_t s_drawCount = 0;

static void DoResize(void)
{

}

static void DrawObjects(HINSTANCE hInstance, HWND hWnd, int currFrameIndex)
{
    // Ensure no more than FRAME_LAG renderings are outstanding
    vkWaitForFences(s_specDevice, 1, &s_presentFences[currFrameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(s_specDevice, 1, &s_presentFences[currFrameIndex]);

    uint32_t currImageIndex = 0;
    VkResult res;
    do
    {
        // Get the index of the next available swapchain image:
        res = vkAcquireNextImageKHR(s_specDevice, s_swapchain, UINT64_MAX, s_imageAcquiredSemaphores[currFrameIndex], VK_NULL_HANDLE, &currImageIndex);
        switch (res)
        {
        case VK_SUCCESS:
            break;

        case VK_ERROR_OUT_OF_DATE_KHR:
            // s_swapchain is out of date (e.g. the window was resized) and must be recreated:
            DoResize();
            break;

        case VK_SUBOPTIMAL_KHR:
            // s_swapchain is not as optimal as it could be,
            // but the platform's presentation engine will still present the image correctly.
            break;

        case VK_ERROR_SURFACE_LOST_KHR:
            if (!CreateVulkanSurface(hInstance, hWnd)) return;
            DoResize();
            break;

        default:
            fprintf(stderr, "vkAcquireNextImageKHR failed: %d\n", res);
            return;
        }
    }
    while (res != VK_SUCCESS);

    if (!UpdateUniformData(currImageIndex)) {
        return;
    }

    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    const VkPipelineStageFlags pipelineStageFlags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &s_imageAcquiredSemaphores[currFrameIndex],
        .pWaitDstStageMask = pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &s_swapchainImageResources[currImageIndex].cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &s_drawCompleteSemaphores[currFrameIndex]
    };
    res = vkQueueSubmit(s_graphicsQueue, 1, &submit_info, s_presentFences[currFrameIndex]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkQueueSubmit failed: %d\n", res);
        return;
    }

    const bool isSeparatePresentQueue = IsSeperatePresentQueue();
    if (isSeparatePresentQueue)
    {
        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signalling the ownership released semaphore when finished
        const VkSubmitInfo presentSubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &s_drawCompleteSemaphores[currFrameIndex],
            .pWaitDstStageMask = pipelineStageFlags,
            .commandBufferCount = 1,
            .pCommandBuffers = &s_swapchainImageResources[currImageIndex].graphics_to_present_cmd_buf,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &s_imageOwnershipSemaphores[currFrameIndex]
        };
        res = vkQueueSubmit(s_presentQueue, 1, &presentSubmitInfo, VK_NULL_HANDLE);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkQueueSubmit for presentation failed: %d\n", res);
            return;
        }
    }

    // If we are using separate queues we have to wait for image ownership,
    // otherwise wait for draw complete
    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = isSeparatePresentQueue ? &s_imageOwnershipSemaphores[currFrameIndex] : &s_drawCompleteSemaphores[currFrameIndex],
        .swapchainCount = 1,
        .pSwapchains = &s_swapchain,
        .pImageIndices = &currImageIndex,
        .pResults = NULL
    };

    VkRectLayerKHR rect;
    VkPresentRegionKHR region;
    VkPresentRegionsKHR regions;
    if (s_supportIncrementalPresent)
    {
        // If using VK_KHR_incremental_present, we provide a hint of the region
        // that contains changed content relative to the previously-presented
        // image.  The implementation can use this hint in order to save
        // work/power (by only copying the region in the hint).  The
        // implementation is free to ignore the hint though, and so we must
        // ensure that the entire image has the correctly-drawn content.
        const uint32_t eighthOfWidth = s_render_width / 8;
        const uint32_t eighthOfHeight = s_render_height / 8;

        rect.offset.x = eighthOfWidth;
        rect.offset.y = eighthOfHeight;
        rect.extent.width = eighthOfWidth * 6;
        rect.extent.height = eighthOfHeight * 6;
        rect.layer = 0;

        region.rectangleCount = 1;
        region.pRectangles = &rect;

        regions.sType = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
        regions.pNext = present.pNext;
        regions.swapchainCount = present.swapchainCount;
        regions.pRegions = &region;

        present.pNext = &regions;
    }

    res = vkQueuePresentKHR(s_presentQueue, &present);
    switch (res)
    {
    case VK_SUCCESS:
        break;

    case VK_ERROR_OUT_OF_DATE_KHR:
        // s_swapchain is out of date (e.g. the window was resized) and must be recreated:
        DoResize();
        break;

    case VK_SUBOPTIMAL_KHR:
        // s_swapchain is not as optimal as it could be, but the platform's presentation engine will still present the image correctly.
        break;

    case VK_ERROR_SURFACE_LOST_KHR:
        if (!CreateVulkanSurface(hInstance, hWnd)) return;
        DoResize();
        break;

    default:
        fprintf(stderr, "vkQueuePresentKHR failed: %d\n", res);
        break;
    }

    // Fetch the timestamp query result
    uint64_t timestamps[MAX_SWAPCHAIN_IMAGE_COUNT * 2] = { 0 };
    res = vkGetQueryPoolResults(s_specDevice, s_timestampQueryPool, 0, 2 * s_swapchainImageCount, 
                        sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT);
    if (res == VK_SUCCESS)
    {
        uint64_t sumDuration = 0;
        for (uint32_t i = 0; i < s_swapchainImageCount; ++i) {
            sumDuration += timestamps[i * 2 + 1] - timestamps[i * 2];
        }
        s_currGPUDuration = (double)sumDuration * (double)s_gpuTimestampPeriod / 1000'000.0;
    }
    else if(res == VK_NOT_READY){
        puts("vkGetQueryPoolResults: query not ready...");
    }

    // Fetch the occlusion query result
    uint64_t occlusions[MAX_SWAPCHAIN_IMAGE_COUNT] = { 0 };
    res = vkGetQueryPoolResults(s_specDevice, s_occlusionQueryPool, 0, s_swapchainImageCount,
        sizeof(occlusions), occlusions, sizeof(occlusions[0]), VK_QUERY_RESULT_64_BIT);
    if (res == VK_SUCCESS)
    {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < s_swapchainImageCount; ++i) {
            sum += occlusions[i];
        }
        s_currOcclusionCount = sum / s_swapchainImageCount;
    }

    ++s_drawCount;
}

static void RunTheRendering(HINSTANCE hInstance, HWND hWnd, int currFrameIndex)
{
    if (!s_isRenderPrepared) return;

    DrawObjects(hInstance, hWnd, currFrameIndex);
}

static void DestroyVulkanAssets(void)
{
    vkDeviceWaitIdle(s_specDevice);

    // Wait for fences from present operations
    for (int i = 0; i < FRAME_LAG; i++)
    {
        if (s_presentFences[i] != VK_NULL_HANDLE)
        {
            vkWaitForFences(s_specDevice, 1, &s_presentFences[i], VK_TRUE, UINT64_MAX);
            vkDestroyFence(s_specDevice, s_presentFences[i], NULL);
        }
        if (s_imageAcquiredSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(s_specDevice, s_imageAcquiredSemaphores[i], NULL);
        }
        if (s_drawCompleteSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(s_specDevice, s_drawCompleteSemaphores[i], NULL);
        }
        if (s_imageOwnershipSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(s_specDevice, s_imageOwnershipSemaphores[i], NULL);
        }
    }

    if (s_descPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(s_specDevice, s_descPool, NULL);
    }
    for (size_t i = 0; i < sizeof(s_pipelines) / sizeof(s_pipelines[0]); ++i)
    {
        if (s_pipelineCaches[i] != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(s_specDevice, s_pipelineCaches[i], NULL);
        }
        if (s_pipelines[i] != VK_NULL_HANDLE) {
            vkDestroyPipeline(s_specDevice, s_pipelines[i], NULL);
        }
    }
    if (s_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(s_specDevice, s_render_pass, NULL);
    }
    if (s_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(s_specDevice, s_pipelineLayout, NULL);
    }
    if (s_descSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(s_specDevice, s_descSetLayout, NULL);
    }

    for (uint32_t i = 0; i < s_swapchainImageCount; ++i)
    {
        if (s_swapchainImageResources[i].framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(s_specDevice, s_swapchainImageResources[i].framebuffer, NULL);
        }
        if (s_swapchainImageResources[i].view != VK_NULL_HANDLE) {
            vkDestroyImageView(s_specDevice, s_swapchainImageResources[i].view, NULL);
        }
        if (s_swapchainImageResources[i].msaaImage != VK_NULL_HANDLE) {
            vkDestroyImage(s_specDevice, s_swapchainImageResources[i].msaaImage, NULL);
        }
        if (s_swapchainImageResources[i].msaaView != VK_NULL_HANDLE) {
            vkDestroyImageView(s_specDevice, s_swapchainImageResources[i].msaaView, NULL);
        }
        if (s_swapchainImageResources[i].cmd_buf != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(s_specDevice, s_commandPool, 1, &s_swapchainImageResources[i].cmd_buf);
        }
    }
    if (s_msaaColorImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_msaaColorImageMemory, NULL);
    }
    if (s_uniformBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_uniformBuffer, NULL);
    }
    if (s_uniformMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_uniformMemory, NULL);
    }
    if (s_vertexCoordsBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_vertexCoordsBuffer, NULL);
    }
    if (s_textureCoordsBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_textureCoordsBuffer, NULL);
    }
    if (s_colorBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_colorBuffer, NULL);
    }
    if (s_vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_vertexMemory, NULL);
    }
    if (s_hostVertexAndUniformBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_hostVertexAndUniformBuffer, NULL);
    }
    if (s_hostVertexUniformMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_hostVertexUniformMemory, NULL);
    }
    if (s_hostUploadTextureBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(s_specDevice, s_hostUploadTextureBuffer, NULL);
    }
    if (s_hostUploadTextureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_hostUploadTextureMemory, NULL);
    }
    if (s_textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(s_specDevice, s_textureImageView, NULL);
    }
    if (s_textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(s_specDevice, s_textureImage, NULL);
    }
    if (s_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(s_specDevice, s_textureSampler, NULL);
    }
    if (s_textureMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_textureMemory, NULL);
    }
    if (s_depthResource.image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(s_specDevice, s_depthResource.image_view, NULL);
    }
    if (s_depthResource.image != VK_NULL_HANDLE) {
        vkDestroyImage(s_specDevice, s_depthResource.image, NULL);
    }
    if (s_depthResource.msaaView != VK_NULL_HANDLE) {
        vkDestroyImageView(s_specDevice, s_depthResource.msaaView, NULL);
    }
    if (s_depthResource.msaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(s_specDevice, s_depthResource.msaaImage, NULL);
    }
    if (s_depthResource.device_memory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_depthResource.device_memory, NULL);
    }
    if (s_depthResource.msaaDeviceMemory != VK_NULL_HANDLE) {
        vkFreeMemory(s_specDevice, s_depthResource.msaaDeviceMemory, NULL);
    }
    if (s_timestampQueryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(s_specDevice, s_timestampQueryPool, NULL);
    }
    if (s_occlusionQueryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(s_specDevice, s_occlusionQueryPool, NULL);
    }
    if (s_commandPool != VK_NULL_HANDLE)
    {
        if (s_commandBuffers[0] != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(s_specDevice, s_commandPool, (uint32_t)(sizeof(s_commandBuffers) / sizeof(s_commandBuffers[0])), s_commandBuffers);
        }
        vkDestroyCommandPool(s_specDevice, s_commandPool, NULL);
    }
    if (s_presentCommandPool != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < s_swapchainImageCount; ++i)
        {
            if (s_swapchainImageResources[i].graphics_to_present_cmd_buf != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(s_specDevice, s_presentCommandPool, 1, &s_swapchainImageResources[i].graphics_to_present_cmd_buf);
            }
        }
        vkDestroyCommandPool(s_specDevice, s_presentCommandPool, NULL);
    }
    if (s_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(s_specDevice, s_swapchain, NULL);
    }
    if (s_specDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(s_specDevice, NULL);
    }
    if (s_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(s_instance, s_surface, NULL);
    }
    if (s_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(s_instance, NULL);
    }
}

static int s_currFrameIndex = 0;
static POINT s_wndMinsize;                // minimum window size
static const char s_appName[] = "Vulkan Advanced";

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);
        SetWindowLongA(hWnd, GWL_STYLE, GetWindowLongA(hWnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
        SetWindowLongA(hWnd, GWL_STYLE, GetWindowLongA(hWnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
        SetWindowLongA(hWnd, GWL_STYLE, GetWindowLongA(hWnd, GWL_STYLE) & ~WS_SIZEBOX);
        break;
    }

    case WM_CLOSE:
        DestroyVulkanAssets();
        PostQuitMessage(0);
        break;

    case WM_PAINT:
        RunTheRendering(GetModuleHandleA(NULL), hWnd, s_currFrameIndex++);
        if (s_currFrameIndex == FRAME_LAG) {
            s_currFrameIndex = 0;
        }
        if (s_drawCount % 60 == 0)
        {
            char buffer[64];
            sprintf_s(buffer, sizeof(buffer), "%s -- GPU: %.2f ms | occlusions: %u", s_appName, s_currGPUDuration, (uint32_t)s_currOcclusionCount);
            SetWindowTextA(hWnd, buffer);
        }
        break;

    case WM_GETMINMAXINFO:  // set window's minimum size
        ((MINMAXINFO*)lParam)->ptMinTrackSize = s_wndMinsize;
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        // Resize the application to the new window size, except when
        // it was minimized. Vulkan doesn't support images or swapchains
        // with width=0 and height=0.
        if (wParam != SIZE_MINIMIZED)
        {
            s_render_width = lParam & 0xffff;
            s_render_height = (lParam & 0xffff0000U) >> 16;
            DoResize();
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        case VK_LEFT:
            break;
        case VK_RIGHT:
            break;
        case VK_SPACE:
        case VK_RETURN:
            s_isRotating = !s_isRotating;
            break;
        }
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static HWND CreateAndInitializeWindow(HINSTANCE hInstance, LPCSTR appName, int windowWidth, int windowHeight)
{
    WNDCLASSEXA win_class;
    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = hInstance;
    win_class.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = appName;
    win_class.hIconSm = LoadIconA(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassExA(&win_class))
    {
        // It didn't work, so try to give a useful error:
        fprintf(stderr, "Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    const LONG windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU;

    HWND hWnd = CreateWindowExA(
        0,                            // extra style
        appName,                    // class name
        appName,                   // app name
        windowStyle,                   // window style
        CW_USEDEFAULT, CW_USEDEFAULT,     // x, y coords
        windowWidth,                    // width
        windowHeight,                  // height
        NULL,                        // handle to parent
        NULL,                            // handle to menu
        hInstance,                            // hInstance
        NULL);

    if (hWnd == NULL) {
        // It didn't work, so try to give a useful error:
        puts("Cannot create a window in which to draw!");
    }

    // Window client area size must be at least 1 pixel high, to prevent crash.
    s_wndMinsize.x = GetSystemMetrics(SM_CXMINTRACK);
    s_wndMinsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;

    return hWnd;
}

int main(int argc, const char* const argv[])
{
    if (!InitializeVulkanInstance(s_appName, "ZennyEngine")) {
        return 0;
    }

    if (!InitializeVulkanDevice(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) {
        return 0;
    }

    // Windows Instance
    HINSTANCE wndInstance = GetModuleHandleA(NULL);

    // window handle
    HWND wndHandle = CreateAndInitializeWindow(wndInstance, s_appName, WINDOW_WIDTH, WINDOW_HEIGHT);

    s_render_width = WINDOW_WIDTH;
    s_render_height = WINDOW_HEIGHT;

    // Initialize loop condition variable
    bool done = true;

    do
    {
        if (!CreateVulkanSurface(wndInstance, wndHandle)) break;
        if (!CreateVulkanSwapchain()) break;
        if (!CreateFencesAndSemaphores()) break;
        if (!CreateCommandBufferAndBeginCommand()) break;
        if (!CreateQueryPools()) break;
        if (!CreateVertexAndUniformBuffersAndMemories()) break;
        CopyFromHostToDeviceBuffersAndSync();
        if (!CreateDepthReource()) break;
        if (!CreateDescriptorSetAndPipelineLayout()) break;
        if (!CreateRenderPass()) break;
        
        const char* vsSPV = s_supportFragmentShadingRate ? "shaders/fsr.vert.spv" : "shaders/flatten.vert.spv";
        const char* fsSPV = s_supportFragmentShadingRate ? "shaders/fsr.frag.spv" : "shaders/flatten.frag.spv";
        if (!CreateGraphicsPipeline(vsSPV, fsSPV, FLATTEN_PIPELINE_INDEX)) break;
        if (!CreateGraphicsPipeline("shaders/gradient.vert.spv", "shaders/gradient.frag.spv", GRAIENT_PIPELINE_INDEX)) break;
        s_pipelines[GEOMETRY_SHADER_PIPELINE_INDEX] = CreateGeometryShaderGraphicsPipeline(s_specDevice, "shaders/geomtest.vert.spv", "shaders/geomtest.frag.spv", "shaders/geomtest.geom.spv",
            s_pipelineLayout, s_render_pass, &s_pipelineCaches[GEOMETRY_SHADER_PIPELINE_INDEX]);
        if (s_pipelines[GEOMETRY_SHADER_PIPELINE_INDEX] == VK_NULL_HANDLE) break;
        if (!CreateTexturePipelineAssets(s_currPhysicalDevice, s_specDevice, s_graphicsQueueFamilyIndex, s_commandBuffers[0], "shaders/texture.vert.spv", "shaders/texture.frag.spv",
            s_pipelineLayout, s_render_pass, &s_textureImage, &s_textureImageView, &s_textureSampler, &s_hostUploadTextureBuffer, &s_hostUploadTextureMemory, &s_textureMemory,
            &s_pipelineCaches[TEXTURE_PIPELINE_INDEX], &s_pipelines[TEXTURE_PIPELINE_INDEX])) {
            break;
        }
        if (dyn_vkCmdDrawMeshTasksEXT != NULL)
        {
            s_pipelines[MESH_SHADER_PIPELINE_INDEX] = CreateMeshShaderGraphicsPipeline(s_specDevice, "shaders/basic_ms.task.spv", "shaders/basic_ms.mesh.spv", "shaders/basic_ms.frag.spv",
                                                                                    s_pipelineLayout, s_render_pass, &s_pipelineCaches[MESH_SHADER_PIPELINE_INDEX]);
            if (s_pipelines[MESH_SHADER_PIPELINE_INDEX] == VK_NULL_HANDLE) break;
        }
        if (!CreateDescriptorPoolAndSet()) break;
        if (!CreateFramebuffers()) break;
        
        for (uint32_t i = 0; i < s_swapchainImageCount; ++i)
        {
            if (!RecordCommandsForDraw(s_swapchainImageResources[i].cmd_buf, i)) {
                break;
            }
        }

        s_isRenderPrepared = true;

        // Prepare functions above may generate pipeline commands that need to be flushed before beginning the render loop.
        if (!FlushInitCommand()) break;

        done = false;
    }
    while (false);

    // main message loop
    MSG msg;
    while (!done)
    {
        PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)  // check for a quit message
        {
            done = true;  // if found, quit app
        }
        else
        {
            // Translate and dispatch to event queue
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        RedrawWindow(wndHandle, NULL, NULL, RDW_INTERNALPAINT);
    }

    if (wndHandle != NULL)
    {
        DestroyWindow(wndHandle);
        wndHandle = NULL;
    }
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

