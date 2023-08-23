global render_group debugRenderGroup;
global render_group uiRenderGroup;

global font_engine g_fontEngine;
global u32 texture, g_bgTexture, g_emptyTexture;

internal v4 clearBackground = RGBToFloat(LIGHTGRAY);

vertex vertices[] = 
{
	Vertex(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(1.0f,  1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
	Vertex(-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
};

u32 indices[] = 
{
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
};

internal void 
MakeTexture(u32 num, u32 * textureIds)
{
	
}

internal void 
PushDataToTexture(u32 texture, u32 width, u32 height, u8 * data)
{
	
}

internal void 
PushDataToTextureRGB(u32 texture, u32 width, u32 height, u8 * data)
{
	
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL 
debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, 
			 uint64_t object, size_t location, int32_t messageCode, 
			 const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	
	char buffer[1024];
    sprintf(buffer, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	
	// TODO(Ecy): HAHHAHAHAHAH, no ...
	printf(buffer);
    return VK_FALSE;
}

internal b32 
IsExtensionAvailable(const VkExtensionProperties *properties, u32 count, const char* extension)
{
    for(u32 index = 0;
		index < count;
		++index)
	{
        if(strcmp(properties[index].extensionName, extension) == 0)
			return true;
	}
    return false;
}

internal void
CreateDeviceContext(vk_render_context *context, char **extensions, u32 *extensionCount)
{
	VkResult result;
	
	// Create Vulkan instance
	{
		extensions[(*extensionCount)++] = VK_KHR_SURFACE_EXTENSION_NAME;
		
		VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; 
		
		// Enumerate available extensions
		u32 propertiesCount;
		VkExtensionProperties properties[16];
		vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, nullptr);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &propertiesCount, properties);
		CheckRes(result);
		
		// Enable required extensions (???)
		if(IsExtensionAvailable(properties, propertiesCount, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			extensions[(*extensionCount)++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
		}
		
		// Enable validation layer
#if DEBUG
		const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
		instanceCreateInfo.enabledLayerCount = 1;
		instanceCreateInfo.ppEnabledLayerNames = layers;
		extensions[(*extensionCount)++] = "VK_EXT_debug_report";
#endif
		
		// Create vulkan instance
		instanceCreateInfo.enabledExtensionCount   = *extensionCount;
		instanceCreateInfo.ppEnabledExtensionNames = extensions;
		
		result = vkCreateInstance(&instanceCreateInfo, NULL, &context->instance);
		CheckRes(result);
	
		auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
			vkGetInstanceProcAddr(context->instance, "vkCreateDebugReportCallbackEXT");
        Assert(vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | 
			VK_DEBUG_REPORT_WARNING_BIT_EXT | 
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        result = vkCreateDebugReportCallbackEXT(context->instance, &debug_report_ci, nullptr, &context->debugReport);
        CheckRes(result);
		
	}
	
	// Select physical device
	{
		u32 gpuCount;
		VkResult result = vkEnumeratePhysicalDevices(context->instance, &gpuCount, nullptr);
		CheckRes(result);
		Assert(gpuCount > 0);
		
		VkPhysicalDevice gpus[16];
		result = vkEnumeratePhysicalDevices(context->instance, &gpuCount, gpus);
		CheckRes(result);
		
		// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
		// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
		// dedicated GPUs) is out of scope of this sample.
		
		/* 
				for (VkPhysicalDevice& device : gpus)
				{
					VkPhysicalDeviceProperties properties;
					vkGetPhysicalDeviceProperties(device, &properties);
					if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						return device;
				}
		*/
		
		// Use first GPU (Integrated) is a Discrete one is not available.
		if (gpuCount > 0)
		{
			context->physicalDevice = gpus[0]; 
		}
	}
	
	// Select graphics queue family
	{
		context->queueFamily = (u32) - 1;
		u32 queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &queueCount, nullptr);
		VkQueueFamilyProperties queues[16];
		vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &queueCount, queues);
		for (u32 index = 0; 
			 index < queueCount;
			 ++index)
		{
			if(queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				context->queueFamily = index;
				break;
			}
			
			if(queues[index].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
			{
				context->queueDecodeFamily = index;
				context->queueDecodeNums   = queues[index].queueCount;
			}
		}
		
		Assert(context->queueFamily != (u32) - 1);
	}
	
	// Create logical device
	{
		char *deviceExtensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			"VK_KHR_sampler_ycbcr_conversion",
			VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_KHR_VIDEO_QUEUE_EXTENSION_NAME,
			VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME,
			VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME,
			VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME,
		};
		
		u32 extSize = 10;
		
		u32 propertiesCount;
		VkExtensionProperties properties[256];
		vkEnumerateDeviceExtensionProperties(context->physicalDevice, nullptr, &propertiesCount, nullptr);
		vkEnumerateDeviceExtensionProperties(context->physicalDevice, nullptr, &propertiesCount, properties);
		
		for(u32 index = 0;
			index < extSize;
			++index)
		{
			b32 isIncluded = false;
			for(u32 extIndex = 0;
				extIndex < propertiesCount;
				++extIndex)
			{
				if(!strcmp(properties[extIndex].extensionName, deviceExtensions[index]))
				{
					isIncluded = true;
					break;
				} 
			}
			
			Assert(isIncluded);
		}
		
		r32 queuePriority[] = { 1.0f };
		VkDeviceQueueCreateInfo queueInfo[2] = {};
		queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo[0].queueFamilyIndex = context->queueFamily;
		queueInfo[0].queueCount = 1;
		queueInfo[0].pQueuePriorities = queuePriority;
		
		queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo[1].queueFamilyIndex = context->queueDecodeFamily;
		queueInfo[1].queueCount = context->queueDecodeNums;
		queueInfo[1].pQueuePriorities = queuePriority;
		
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = ArrSize(queueInfo);
		createInfo.pQueueCreateInfos = queueInfo;
		createInfo.enabledExtensionCount = extSize;
		createInfo.ppEnabledExtensionNames = deviceExtensions;
		
		result = vkCreateDevice(context->physicalDevice, &createInfo, nullptr, &context->device);
		CheckRes(result);
		
		vkGetDeviceQueue(context->device, context->queueFamily, 0, &context->queue);
		
		for(u32 index = 0;
			index < context->queueDecodeNums;
			++index)
		{
			vkGetDeviceQueue(context->device, context->queueDecodeFamily, 0, &context->decodeQueue[index]);
		}
	}
	
	// Create descriptor pool
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = poolSizes;
		result = vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &context->descriptorPool);
		CheckRes(result);
	}
}

internal void
CreateSwapchain(vk_render_context *context)
{
	VkResult result;
	
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, context->surface, &capabilities);
	
	// Figure out a suitable surface transform.
	VkSurfaceTransformFlagBitsKHR pre_transform;
	if(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = capabilities.currentTransform;
	}
	
	// Find a supported composite type.
	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	else if(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
	{
		composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	}
	
	context->extent = capabilities.currentExtent;
	
	VkSwapchainCreateInfoKHR createSwapChainInfo{};
	createSwapChainInfo.sType               = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createSwapChainInfo.surface             = context->surface;
	createSwapChainInfo.minImageCount       = capabilities.minImageCount + 1;
	createSwapChainInfo.imageFormat         = context->surfaceFormat.format;
	createSwapChainInfo.imageColorSpace     = context->surfaceFormat.colorSpace;
	createSwapChainInfo.imageExtent         = context->extent;
	createSwapChainInfo.imageArrayLayers    = 1;
	createSwapChainInfo.imageUsage          = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createSwapChainInfo.imageSharingMode    = VK_SHARING_MODE_EXCLUSIVE;
	createSwapChainInfo.preTransform        = pre_transform;
	createSwapChainInfo.compositeAlpha      = composite;
	createSwapChainInfo.presentMode         = context->presentMode;
	createSwapChainInfo.clipped             = VK_TRUE;
	
	result = vkCreateSwapchainKHR(context->device, &createSwapChainInfo, nullptr, &context->swapchain);
	CheckRes(result);
	
	result = vkGetSwapchainImagesKHR(context->device, context->swapchain, &context->imageCount, nullptr);
	CheckRes(result);
	
	Assert(context->imageCount > 0);
	
	VkImage images[16];
	result = vkGetSwapchainImagesKHR(context->device, context->swapchain, &context->imageCount, images);
	CheckRes(result);
	
	{
		VkAttachmentDescription attachment = {};
		attachment.format         = context->surfaceFormat.format;
		attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		
		attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		
		VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments    = &color_attachment;
		
		VkSubpassDependency dependency = {};
		dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass          = 0;
		dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		rp_info.attachmentCount        = 1;
		rp_info.pAttachments           = &attachment;
		rp_info.subpassCount           = 1;
		rp_info.pSubpasses             = &subpass;
		rp_info.dependencyCount        = 1;
		rp_info.pDependencies          = &dependency;
		
		result = vkCreateRenderPass(context->device, &rp_info, nullptr, &context->renderpass);
		CheckRes(result);
	}
	
	VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	
	VkImageViewCreateInfo viewInfo = {};
	{
		viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                      = context->surfaceFormat.format;
		
		VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewInfo.subresourceRange            = image_range;
		
		viewInfo.components.r                = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g                = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b                = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a                = VK_COMPONENT_SWIZZLE_A;
	}
	
	VkFramebufferCreateInfo fb_info = {}; 
	VkImageView attachment[1];
	{
		
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass      = context->renderpass;
		fb_info.pAttachments    = attachment;
		fb_info.attachmentCount = 1;
		fb_info.width           = context->extent.width;
		fb_info.height          = context->extent.height;
		fb_info.layers          = 1;
		
	}
	
	for(u32 imageIndex = 0;
		imageIndex < context->imageCount;
		++imageIndex)
	{
		vk_per_frame *frame = &context->perFrame[imageIndex];
		
		frame->backbuffer = images[imageIndex];
			
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		result = vkCreateFence(context->device, &info, nullptr, &frame->fence);
		CheckRes(result);
		
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPoolInfo.queueFamilyIndex   = context->queueFamily;
		result = vkCreateCommandPool(context->device, &cmdPoolInfo, nullptr, 
									 &frame->commandPool);
		CheckRes(result);
		
		VkCommandBufferAllocateInfo cmdBufInfo = {};
		cmdBufInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufInfo.commandPool          = frame->commandPool;
		cmdBufInfo.level                = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufInfo.commandBufferCount   = 1;
		result = vkAllocateCommandBuffers(context->device, &cmdBufInfo, &frame->commandBuffer);
		CheckRes(result);
		
		// Create backbuffer views
		viewInfo.image = frame->backbuffer;
		result = vkCreateImageView(context->device, &viewInfo, nullptr, &frame->backbufferView);
		CheckRes(result);
		
		attachment[0] = frame->backbufferView;
		result = vkCreateFramebuffer(context->device, &fb_info, nullptr, &frame->framebuffer);
		CheckRes(result);
		
		vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &context->semaphores[imageIndex].imageAcquiredSemaphore);
		CheckRes(result);
		
		vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &context->semaphores[imageIndex].renderCompleteSemaphore);
		CheckRes(result);
	}
	
}

internal void
CreatePipeline(vk_render_context *context, app_state *appContext)
{
	VkResult result;
	
	// Create pipeline
	VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	result = vkCreatePipelineLayout(context->device, &layout_info, nullptr, &context->pipeLayout);
	CheckRes(result);
	
	VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	raster.cullMode  = VK_CULL_MODE_BACK_BIT;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;
	
	VkPipelineColorBlendAttachmentState blend_attachment{};
	blend_attachment.colorWriteMask = 
		VK_COLOR_COMPONENT_R_BIT | 
		VK_COLOR_COMPONENT_G_BIT | 
		VK_COLOR_COMPONENT_B_BIT | 
		VK_COLOR_COMPONENT_A_BIT;
	
	VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	blend.attachmentCount = 1;
	blend.pAttachments    = &blend_attachment;
	
	VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewport.viewportCount = 1;
	viewport.scissorCount  = 1;
	
	VkPipelineDepthStencilStateCreateInfo depth_stencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	
	VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	
	VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	
	VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	dynamic.pDynamicStates    = dynamics;
	dynamic.dynamicStateCount = 2;
	
	VkPipelineShaderStageCreateInfo shader_stages = {};
	
	debug_read_file_result vertFile = appContext->DEBUGPlatformReadEntireFile(NULL, "data/vert.spv");
	debug_read_file_result fragFile = appContext->DEBUGPlatformReadEntireFile(NULL, "data/frag.spv");
	Assert(vertFile.contentSize > 0);
	Assert(fragFile.contentSize > 0);
	
	VkShaderModuleCreateInfo vertCreateInfo = {};
	
	vertCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertCreateInfo.codeSize = vertFile.contentSize;
	vertCreateInfo.pCode    = (u32*)vertFile.contents;
	
	VkShaderModule vertShader;
	result = vkCreateShaderModule(context->device, &vertCreateInfo, nullptr, &vertShader);
	CheckRes(result);
	
	VkShaderModuleCreateInfo fragCreateInfo{};
	
	fragCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragCreateInfo.codeSize = fragFile.contentSize;
	fragCreateInfo.pCode    = (u32*)fragFile.contents;
	
	VkShaderModule fragShader;
	result = vkCreateShaderModule(context->device, &fragCreateInfo, nullptr, &fragShader);
	CheckRes(result);
	
	VkPipelineShaderStageCreateInfo vertStageInfo = {};
	vertStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertShader;
	vertStageInfo.pName  = "main";
	
	VkPipelineShaderStageCreateInfo fragStageInfo = {};
	fragStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragShader;
	fragStageInfo.pName  = "main";
	
	VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};
	
	VkGraphicsPipelineCreateInfo pipe{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	pipe.stageCount          = 2;
	pipe.pStages             = shaderStages;
	pipe.pVertexInputState   = &vertex_input;
	pipe.pInputAssemblyState = &input_assembly;
	pipe.pRasterizationState = &raster;
	pipe.pColorBlendState    = &blend;
	pipe.pMultisampleState   = &multisample;
	pipe.pViewportState      = &viewport;
	pipe.pDepthStencilState  = &depth_stencil;
	pipe.pDynamicState       = &dynamic;
	
	pipe.renderPass = context->renderpass;
	pipe.layout     = context->pipeLayout;
	
	result = vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipe, nullptr, &context->pipeline);
	CheckRes(result);
	
	vkDestroyShaderModule(context->device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(context->device, shaderStages[1].module, nullptr);
}

internal void
CreateVideoSession()
{
	
}

internal void 
UpdateBackgroundTexture()
{
	
}

internal void 
Render(vk_render_context *context, app_state *appContext)
{
	VkSemaphore imageAcquiredSemaphore  = context->semaphores[context->semaphoreIndex].imageAcquiredSemaphore;
    VkSemaphore renderCompleteSemaphore = context->semaphores[context->semaphoreIndex].renderCompleteSemaphore;
	
	VkResult res = vkAcquireNextImageKHR(context->device, context->swapchain, UINT64_MAX, 
										 imageAcquiredSemaphore, VK_NULL_HANDLE, &context->frameIndex);
	
	if(res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//res = AcquireNextImage(context, &index);
		// Rebuild swap chain?
		context->swapChainRebuild = true;
	}
	CheckRes(res);
	
	vk_per_frame *frame = &context->perFrame[context->frameIndex];
	
	{
		res = vkWaitForFences(context->device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
		CheckRes(res);
		
		res = vkResetFences(context->device, 1, &frame->fence);
		CheckRes(res);
	}
	{
		res = vkResetCommandPool(context->device, frame->commandPool, 0);
		CheckRes(res);
		
		VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		res = vkBeginCommandBuffer(frame->commandBuffer, &info);
		CheckRes(res);
	}
	{
		VkClearValue clearValue;
		clearValue.color = {{0.01f, 0.01f, 0.033f, 1.0f}};
		
		VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = context->renderpass;
        info.framebuffer = frame->framebuffer;
        info.renderArea.extent = context->extent;
        info.clearValueCount = 1;
        info.pClearValues = &clearValue;
        vkCmdBeginRenderPass(frame->commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}
	
	// Render Triangle
	{
		vkCmdBindPipeline(frame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);
		
		VkViewport vp = {};
		vp.width    = context->extent.width;
		vp.height   = context->extent.height;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		// Set viewport dynamically
		vkCmdSetViewport(frame->commandBuffer, 0, 1, &vp);
		
		VkRect2D scissor = {};
		scissor.extent = context->extent;
		// Set scissor dynamically
		vkCmdSetScissor(frame->commandBuffer, 0, 1, &scissor);
		
		// Actual draw command
		vkCmdDraw(frame->commandBuffer, 3, 1, 0, 0);
		
		// Complete render pass
		vkCmdEndRenderPass(frame->commandBuffer);
		{
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &imageAcquiredSemaphore;
			info.pWaitDstStageMask = &wait_stage;
			info.commandBufferCount = 1;
			info.pCommandBuffers = &frame->commandBuffer;
			info.signalSemaphoreCount = 1;
			info.pSignalSemaphores = &renderCompleteSemaphore;
			
			res = vkEndCommandBuffer(frame->commandBuffer);
			CheckRes(res);
			res = vkQueueSubmit(context->queue, 1, &info, frame->fence);
			CheckRes(res);
		}
	}
	
	// Present Image
	{
		// TODO(Ecy): sync issues with the first few presented frames:
		// Validation error: pSwapChains[0] images passed to present must be in layout VK_IMAGE_LAYOUT_PRESENT_SRC_KHR [...]
		// It could be due to the order in which they are presented ...
		if(context->swapChainRebuild) return;
		
		VkSemaphore render_complete_semaphore = context->semaphores[context->semaphoreIndex].renderCompleteSemaphore;
		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &render_complete_semaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &context->swapchain;
		info.pImageIndices = &context->frameIndex;
		res = vkQueuePresentKHR(context->queue, &info);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
		{
			context->swapChainRebuild = true;
			return;
		}
		CheckRes(res);
		
		context->semaphoreIndex = (context->semaphoreIndex + 1) % context->imageCount;
	}
	
}
