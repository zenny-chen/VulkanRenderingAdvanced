#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32
// ATTENTION: <Windows.h> MUST BE included ahead of <vulkan/vulkan_win32.h>
#include <Windows.h>
#include <wincodec.h>
#include <vulkan/vulkan_win32.h>

static inline FILE* GeneralOpenFile(const char* path)
{
    FILE* fp = NULL;
    // error code such as `EISDIR`, `EMFILE`, `ENAMETOOLONG`, etc.
    const errno_t errCode = fopen_s(&fp, path, "rb");
    if (errCode != 0)
    {
        printf("Open file '%s' failed, because: %d\n", path, errCode);
        return NULL;
    }
    return fp;
}

#define _USE_MATH_DEFINES

#else

static inline FILE* GeneralOpenFile(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
    {
        printf("File '%s' open failed!\n", path);
        return NULL;
    }
    return fp;
}

#endif // _WIN32

#include <math.h>

enum
{
    VERTEX_BUFFER_LOCATION_INDEX,
    COLOR_BUFFER_LOCATION_INDEX,
    TEXCOORDS_BUFFER_LOCATION_INDEX
};

extern bool CreateShaderModule(const char* fileName, VkShaderModule* pShaderModule);

extern bool CreateTexturePipelineAssets(VkPhysicalDevice currPhysicalDevice, VkDevice specDevice, uint32_t graphicsQueueFamilyIndex, VkCommandBuffer commandBuffer,
                                        const char* vertSPVFilePath, const char* fragSPVFilePath, VkPipelineLayout pipelineLayout, VkRenderPass renderPass,
                                        VkImage* outImage, VkImageView* outImageView, VkSampler* outSampler, VkBuffer* pHostUploadBuffer, VkDeviceMemory* pHostUploadMemory, VkDeviceMemory* pTextureImageMemory,
                                        VkPipelineCache* outPipelineCache, VkPipeline* outPipeline);

extern VkPipeline CreateGeometryShaderGraphicsPipeline(VkDevice specDevice, const char* vertSPVFilePath, const char* fragSPVFilePath, const char* geomSPVFilePath,
                                        VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCache* outPipelineCache);

extern VkPipeline CreateMeshShaderGraphicsPipeline(VkDevice specDevice, const char* taskSPVFilePath, const char* meshSPVFilePath, const char* fragmentSPVFilePath,
                                                    VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCache* outPipelineCache);

