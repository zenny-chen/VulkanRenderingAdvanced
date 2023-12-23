#include "common.h"


VkPipeline CreateGeometryShaderGraphicsPipeline(VkDevice specDevice, const char* vertSPVFilePath, const char* fragSPVFilePath, const char* geomSPVFilePath,
                            VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCache* outPipelineCache)
{
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    VkShaderModule geometryShaderModule = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkPipeline dstPipeline = VK_NULL_HANDLE;
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    do
    {
        if (!CreateShaderModule(vertSPVFilePath, &vertexShaderModule)) break;
        if (!CreateShaderModule(fragSPVFilePath, &fragmentShaderModule)) break;
        if (!CreateShaderModule(geomSPVFilePath, &geometryShaderModule)) break;

        const VkSpecializationMapEntry mapEntries[] = {
            {
                .constantID = 0U,
                .offset = 0U,
                .size = sizeof(float)
            }
        };
        // This specialization constant is for the geometry shader.
        const float edgeWidth = 0.4f;
        const VkSpecializationInfo specializationInfo = {
            .mapEntryCount = 1U,
            .pMapEntries = mapEntries,
            .dataSize = sizeof(float),
            .pData = &edgeWidth
        };

        // three shader stages
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
            },
            // geometry shader
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .stage = VK_SHADER_STAGE_GEOMETRY_BIT,
                .module = geometryShaderModule,
                .pName = "main",
                .pSpecializationInfo = &specializationInfo
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
            // geometry shader test uses point primitive topology while the other tests use triangle strip topology
            .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

        res = vkCreatePipelineCache(specDevice, &pipelineCacheInfo, NULL, &pipelineCache);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreatePipelineCache failed: %d\n", res);
            break;
        }

        res = vkCreateGraphicsPipelines(specDevice, pipelineCache, 1, &pipelineCreateInfo, NULL, &dstPipeline);
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
    if (geometryShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(specDevice, geometryShaderModule, NULL);
    }
    if (fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(specDevice, fragmentShaderModule, NULL);
    }

    if (res == VK_SUCCESS)
    {
        *outPipelineCache = pipelineCache;
        return dstPipeline;
    }

    if (pipelineCache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(specDevice, pipelineCache, NULL);
    }
    if (dstPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(specDevice, dstPipeline, NULL);
    }

    return dstPipeline;
}

