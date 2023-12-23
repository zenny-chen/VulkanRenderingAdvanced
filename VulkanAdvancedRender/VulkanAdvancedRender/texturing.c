#include "common.h"

static VkImage CreateTextureResource(VkPhysicalDevice currPhysicalDevice, VkDevice specDevice, uint32_t textureWidth, uint32_t textureHeight, void* srcImageData, uint32_t graphicsQueueFamilyIndex,
                                    VkImageView *outImageView, VkSampler *outSampler, VkBuffer *outHostUploadBuffer, VkDeviceMemory *outDeviceMemory, VkDeviceMemory *outHostUploadMemory)
{
    VkImage dstImage = VK_NULL_HANDLE;

    VkPhysicalDeviceMemoryProperties memoryProperties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(currPhysicalDevice, &memoryProperties);

    const VkDeviceSize imageBufferSize = textureWidth * textureHeight * 4U;

    const VkBufferCreateInfo hostUploadBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = imageBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex
    };
    VkResult res = vkCreateBuffer(specDevice, &hostUploadBufferCreateInfo, NULL, outHostUploadBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer for host upload buffer failed: %d\n", res);
        return dstImage;
    }

    do
    {
        VkMemoryRequirements hostUploadMemoryRequirements = { 0 };
        vkGetBufferMemoryRequirements(specDevice, *outHostUploadBuffer, &hostUploadMemoryRequirements);

        uint32_t memoryTypeIndex;
        // Find host visible property memory type index
        for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
        {
            if ((hostUploadMemoryRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
                continue;
            }
            const VkMemoryType memoryType = memoryProperties.memoryTypes[memoryTypeIndex];
            if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 &&
                (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0 &&
                memoryProperties.memoryHeaps[memoryType.heapIndex].size >= hostUploadMemoryRequirements.size)
            {
                // found our memory type!
                break;
            }
        }

        const VkMemoryAllocateInfo hostMemAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = hostUploadMemoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };

        res = vkAllocateMemory(specDevice, &hostMemAllocInfo, NULL, outHostUploadMemory);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkAllocateMemory for host vertex and uniform memory failed: %d\n", res);
            break;
        }

        res = vkBindBufferMemory(specDevice, *outHostUploadBuffer, *outHostUploadMemory, 0);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
            break;
        }

        const VkImageCreateInfo imageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent = { textureWidth, textureHeight, 1U },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,      // image with sampler
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = (uint32_t[]){ graphicsQueueFamilyIndex },
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        res = vkCreateImage(specDevice, &imageCreateInfo, NULL, &dstImage);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateImage for texture faild: %d\n", res);
            return dstImage;
        }

        VkMemoryRequirements memoryRequirements = { 0 };
        vkGetImageMemoryRequirements(specDevice, dstImage, &memoryRequirements);

        // Find host visible property memory type index
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

        const VkMemoryAllocateInfo memAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = memoryTypeIndex
        };

        res = vkAllocateMemory(specDevice, &memAllocInfo, NULL, outDeviceMemory);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkAllocateMemory for texture failed: %d\n", res);
            break;
        }

        res = vkBindImageMemory(specDevice, dstImage, *outDeviceMemory, 0);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkBindImageMemory for texture failed: %d\n", res);
            break;
        }

        const VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = dstImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
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

        res = vkCreateImageView(specDevice, &imageViewCreateInfo, NULL, outImageView);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateImageView for texture failed: %d\n", res);
            break;
        }

        const VkSamplerCreateInfo samplerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            .unnormalizedCoordinates = VK_FALSE      // We're going to use normalized coordinates here...
        };

        res = vkCreateSampler(specDevice, &samplerCreateInfo, NULL, outSampler);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateSampler failed: %d\n", res);
            break;
        }

        void* hostData = NULL;
        res = vkMapMemory(specDevice, *outHostUploadMemory, 0U, hostUploadMemoryRequirements.size, 0U, &hostData);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkMapMemory failed for upload texture memory: %d\n", res);
            break;
        }
        memcpy(hostData, srcImageData, imageBufferSize);
        vkUnmapMemory(specDevice, *outHostUploadMemory);

        return dstImage;
    }
    while (false);

    if (*outHostUploadBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(specDevice, *outHostUploadBuffer, NULL);
        *outHostUploadBuffer = VK_NULL_HANDLE;
    }
    if (*outHostUploadMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(specDevice, *outHostUploadMemory, NULL);
        *outHostUploadMemory = NULL;
    }
    if (dstImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(specDevice, dstImage, NULL);
        dstImage = VK_NULL_HANDLE;
    }
    if (*outImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(specDevice, *outImageView, NULL);
        *outImageView = VK_NULL_HANDLE;
    }
    if (*outSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(specDevice, *outSampler, NULL);
        *outSampler = VK_NULL_HANDLE;
    }
    if (*outDeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(specDevice, *outDeviceMemory, NULL);
        *outDeviceMemory = VK_NULL_HANDLE;
    }

    return dstImage;
}

static void CopyImageDataToDeviceTextureBuffer(VkCommandBuffer commandBuffer, VkBuffer hostUploadBuffer, VkImage textureImage, uint32_t textureWidth, uint32_t textureHeight,
                                            uint32_t graphicsQueueFamilyIndex)
{
    VkImageMemoryBarrier imageBarriers[] = {
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = graphicsQueueFamilyIndex,
            .dstQueueFamilyIndex = graphicsQueueFamilyIndex,
            .image = textureImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0U,
                .levelCount = 1U,
                .baseArrayLayer = 0U,
                .layerCount = 1U
            }
        }
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, NULL, 0, NULL, (uint32_t)(sizeof(imageBarriers) / sizeof(imageBarriers[0])), imageBarriers);

    const VkBufferImageCopy copyRegions[] = {
        {
            .bufferOffset = 0U,
            .bufferRowLength = textureWidth,
            .bufferImageHeight = textureHeight,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .baseArrayLayer = 0U,
                .layerCount = 1U
            },
            .imageOffset = { .x = 0U, .y = 0U, .z = 0U },
            .imageExtent = { .width = textureWidth, .height = textureHeight, .depth = 1U }
        }
    };
    vkCmdCopyBufferToImage(commandBuffer, hostUploadBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)(sizeof(copyRegions) / sizeof(copyRegions[0])), copyRegions);

    imageBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, NULL, 0, NULL, (uint32_t)(sizeof(imageBarriers) / sizeof(imageBarriers[0])), imageBarriers);
}

static VkPipeline CreateGraphicsPipeline(VkDevice specDevice, const char* vertSPVFilePath, const char* fragSPVFilePath, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCache *outPipelineCache)
{
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    VkPipeline dstPipeline = VK_NULL_HANDLE;

    do
    {
        if (!CreateShaderModule(vertSPVFilePath, &vertexShaderModule)) break;
        if (!CreateShaderModule(fragSPVFilePath, &fragmentShaderModule)) break;

        // Just use two shader stages
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
            // texture coords buffer
            {
                .binding = TEXCOORDS_BUFFER_LOCATION_INDEX,
                .stride = sizeof(float[2]),
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
            // inTexCoords attribute
            {
                .location = TEXCOORDS_BUFFER_LOCATION_INDEX,
                .binding = TEXCOORDS_BUFFER_LOCATION_INDEX,
                .format = VK_FORMAT_R32G32_SFLOAT,
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
            .cullMode = VK_CULL_MODE_BACK_BIT,
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
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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

        const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = NULL,
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
            .layout = pipelineLayout,
            .renderPass = renderPass,
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

        VkResult res = vkCreatePipelineCache(specDevice, &pipelineCacheInfo, NULL, outPipelineCache);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreatePipelineCache failed: %d\n", res);
            break;
        }

        res = vkCreateGraphicsPipelines(specDevice, *outPipelineCache, 1, &pipelineCreateInfo, NULL, &dstPipeline);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateGraphicsPipelines failed: %d\n", res);
            break;
        }
    }
    while (false);

    if (vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(specDevice, vertexShaderModule, NULL);
    }
    if (fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(specDevice, fragmentShaderModule, NULL);
    }

    return dstPipeline;
}

static HBITMAP LoadBMPFile(BITMAP* outBitmapInfo)
{
    HBITMAP hBmp = (HBITMAP)LoadImageA(NULL, "images/geom.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hBmp == NULL)
    {
        fprintf(stderr, "Load image file failed!\n");
        return NULL;
    }

    if (GetObjectA(hBmp, sizeof(*outBitmapInfo), outBitmapInfo) == 0)
    {
        fprintf(stderr, "Bitmap get failed!\n");
        return NULL;
    }

    return hBmp;
}

bool CreateTexturePipelineAssets(VkPhysicalDevice currPhysicalDevice, VkDevice specDevice, uint32_t graphicsQueueFamilyIndex, VkCommandBuffer commandBuffer,
                                const char* vertSPVFilePath, const char* fragSPVFilePath, VkPipelineLayout pipelineLayout, VkRenderPass renderPass,
                                VkImage *outImage, VkImageView *outImageView, VkSampler *outSampler, VkBuffer *pHostUploadBuffer, VkDeviceMemory *pHostUploadMemory, VkDeviceMemory *pTextureImageMemory,
                                VkPipelineCache* outPipelineCache, VkPipeline* outPipeline)
{
    BITMAP bitmapInfo;
    HBITMAP hBitmap = LoadBMPFile(&bitmapInfo);
    if (hBitmap == NULL) return false;

    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    VkBuffer hostUploadBuffer = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    VkDeviceMemory hostUploadMemory = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkImage textureImage = CreateTextureResource(currPhysicalDevice, specDevice, bitmapInfo.bmWidth, bitmapInfo.bmHeight, bitmapInfo.bmBits, graphicsQueueFamilyIndex,
                                                &textureImageView, &textureSampler, &hostUploadBuffer, &textureMemory, &hostUploadMemory);

    if (hBitmap != NULL) {
        DeleteObject(hBitmap);
    }

    if (textureImage == VK_NULL_HANDLE) return false;

    CopyImageDataToDeviceTextureBuffer(commandBuffer, hostUploadBuffer, textureImage, bitmapInfo.bmWidth, bitmapInfo.bmHeight, graphicsQueueFamilyIndex);

    do
    {
        pipeline = CreateGraphicsPipeline(specDevice, vertSPVFilePath, fragSPVFilePath, pipelineLayout, renderPass, &pipelineCache);
        if (pipeline == VK_NULL_HANDLE) break;

        *outImage = textureImage;
        *outImageView = textureImageView;
        *outSampler = textureSampler;
        *pHostUploadBuffer = hostUploadBuffer;
        *pHostUploadMemory = hostUploadMemory;
        *pTextureImageMemory = textureMemory;
        *outPipelineCache = pipelineCache;
        *outPipeline = pipeline;

        return true;
    }
    while (false);

    if (textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(specDevice, textureImage, NULL);
    }
    if (textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(specDevice, textureImageView, NULL);
    }
    if (textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(specDevice, textureSampler, NULL);
    }
    if (hostUploadBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(specDevice, hostUploadBuffer, NULL);
    }
    if (hostUploadMemory != VK_NULL_HANDLE) {
        vkFreeMemory(specDevice, hostUploadMemory, NULL);
    }
    if (pipelineCache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(specDevice, pipelineCache, NULL);
    }
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(specDevice, pipeline, NULL);
    }

    return false;
}

