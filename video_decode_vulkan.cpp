#include "video_decode_vulkan.h"

global VkExtensionProperties 
h264DecodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION 
};

global VkExtensionProperties 
h265DecodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION 
};

global VkExtensionProperties 
h264EncodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION 
};

global VkExtensionProperties 
h265EncodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_SPEC_VERSION 
};

// TODO(Ecy): do some study of chrome subsampling (4:4:4, 4:2:0, etc.)
VkBuffer videobuffer;
VkImageView videobufferView;

VkVideoProfileInfoKHR videoProfile;
VkVideoSessionKHR session;

// TODO(Ecy): refer to VulkanVideoParser.h from the nvidia decoder sample code
internal void
DetectVideoFormat()
{
	//VkParserDetectedVideoFormat detectedFormat;
}

inline VkResult
InitVideoDecoder(vk_render_context *context)
{
	VkExtent2D maxCodedExtent = {};
	
	VkVideoProfileInfoKHR profileInfo = {};
	profileInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
	profileInfo.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
    profileInfo.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR; // Check support
    profileInfo.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR; // Check support
    profileInfo.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
	
	VkVideoSessionCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
	createInfo.pVideoProfile = &profileInfo;
    createInfo.queueFamilyIndex = context->queueFamily; // Careful, is this main render or secondary render
    createInfo.pictureFormat = pictureFormat;
    createInfo.maxCodedExtent = context->extent; // Is this the right choice?
    createInfo.maxDpbSlots = maxReferencePicturesSlotsCount;
    createInfo.maxActiveReferencePictures = maxReferencePicturesActiveCount;
	
	switch(pictureFormat)
	{
		case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
		{
			createInfo.pStdHeaderVersion = &h264DecodeStdExtensionVersion;
		}break;
		case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
		{
			createInfo.pStdHeaderVersion = &h265DecodeStdExtensionVersion;
		}break;
		case VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_EXT:
		case VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_EXT:
		default:
		{
			// NOTE(Ecy): impossible code path
			Assert(false);
		}break;
	}
	
	VkResult result = vkCreateVideoSessionKHR(context->device, &createInfo, nullptr, &session);
	if(result != VK_SUCCESS)
	{
		return result;
	}
	
	u32 memoryRequirementCount;
	VkVideoSessionMemoryRequirementsKHR memoryRequirements[MAX_BOUND_MEMORY];
	
	result = vkGetVideoSessionMemoryRequirementsKHR(context->device, session, 
													&memoryRequirementCount, NULL);
	CheckRes(result);
	Assert(memoryRequirementCount <= MAX_BOUND_MEMORY);
	
	result = vkGetVideoSessionMemoryRequirementsKHR(context->device, session, 
													&memoryRequirementCount, memoryRequirements);
	CheckRes(result);
	
	// Continue to allocate resources after creating video session: VulkanVideoSession.cpp: l.94
}