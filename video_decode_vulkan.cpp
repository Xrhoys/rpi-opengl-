#include "video_decode_vulkan.h"

global const VkExtensionProperties 
h264DecodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION 
};

global const VkExtensionProperties 
h265DecodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION 
};

global const VkExtensionProperties 
h264EncodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H264_ENCODE_SPEC_VERSION 
};

global const VkExtensionProperties 
h265EncodeStdExtensionVersion = 
{ 
	VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_EXTENSION_NAME, 
	VK_STD_VULKAN_VIDEO_CODEC_H265_ENCODE_SPEC_VERSION 
};

// TODO(Ecy): generate it
global const u32 H265_LEVEL_IDC_1_0 = (u32)(1.0 * 30);
global const u32 H265_LEVEL_IDC_2_0 = (u32)(2.0 * 30);
global const u32 H265_LEVEL_IDC_2_1 = (u32)(2.1 * 30);
global const u32 H265_LEVEL_IDC_3_0 = (u32)(3.0 * 30);
global const u32 H265_LEVEL_IDC_3_1 = (u32)(3.1 * 30);
global const u32 H265_LEVEL_IDC_4_0 = (u32)(4.0 * 30);
global const u32 H265_LEVEL_IDC_4_1 = (u32)(4.1 * 30);
global const u32 H265_LEVEL_IDC_5_0 = (u32)(5.0 * 30);
global const u32 H265_LEVEL_IDC_5_1 = (u32)(5.1 * 30);
global const u32 H265_LEVEL_IDC_5_2 = (u32)(5.2 * 30);
global const u32 H265_LEVEL_IDC_6_0 = (u32)(6.0 * 30);
global const u32 H265_LEVEL_IDC_6_1 = (u32)(6.1 * 30);
global const u32 H265_LEVEL_IDC_6_2 = (u32)(6.2 * 30);

// TODO(Ecy): do some study of chrome subsampling (4:4:4, 4:2:0, etc.)
VkBuffer videobuffer;
VkImageView videobufferView;

VkVideoProfileInfoKHR videoProfile;
VkVideoSessionKHR session = {};

VkDeviceMemory memoryBound[MAX_BOUND_MEMORY];

internal void
DecodeVideo()
{
	// 1 - OUTSIDE OF decoding loop
	//     Create video_decode_frame array (frame buffer count = 10), initialize frame index = 0
	
	
}

internal void
LoadVulkanVideoContext(video_decode_vulkan *decoder, char *filename)
{
	if(avformat_open_input(&decoder->formatContext, filename, NULL, 0) != 0)
	{
		// Couldn't open file
		return;
	}
	
	if(avformat_find_stream_info(decoder->formatContext, NULL) < 0)
	{
		// Couldn't find stream information
		return;
	}
	
	AVCodecParameters *pCodecParam = NULL;
	
	decoder->streamIndex = -1;
	for(u32 index = 0;
		index < decoder->formatContext->nb_streams;
		++index)
	{
		AVCodecParameters *pLocalCodecParameters = decoder->formatContext->streams[index]->codecpar;
		const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
		
		if(pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			decoder->streamIndex = index;
			decoder->codec = (AVCodec*)pLocalCodec;
			pCodecParam = pLocalCodecParameters;
			break;
		}
	}
	if(decoder->streamIndex == -1)
	{
		// NO video stream found.
		return;
	}
	
	decoder->codecContext = avcodec_alloc_context3(decoder->codec);
	
	if(decoder->codecContext == NULL)
	{
		// Copy context failed
		return;
	}
	
	if(avcodec_parameters_to_context(decoder->codecContext, pCodecParam) < 0)
	{
		// Copy context failed
		return;
	}
	
	if(avcodec_open2(decoder->codecContext, decoder->codec, NULL) < 0)
	{
		// Could not open codec
		return;
	}
	
	decoder->pFrame    = av_frame_alloc();
	decoder->packet    = av_packet_alloc();
	
	//auto descriptor = av_pix_fmt_desc_get(AV_PIX_FMT_NV12);
	
	decoder->isLoaded = true;
}

// TODO(Ecy): refer to VulkanVideoParser.h from the nvidia decoder sample code
internal void
DetectVideoFormat()
{
	//VkParserDetectedVideoFormat detectedFormat;
}

internal void
GetSupportedVideoFormats(vk_render_context *context, video_decode_vulkan *decoder)
{
	VkResult res = VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR;
	
	
}

internal VkResult
InitVideoDecoder(vk_render_context *context, video_decode_vulkan *decoder)
{
	if(decoder->codec->id & (AV_CODEC_ID_H265) == 0)
	{
		// Unsupported decoding format
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}
	
	{
		// First extract the stream itself
		
	}
	
	// Get video capabilities
	VkVideoProfileInfoKHR profileInfo = {};
	profileInfo.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
	profileInfo.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
    profileInfo.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR; // Check support
    profileInfo.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR; // Check support
    profileInfo.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
	VkVideoCapabilitiesKHR capabilities = {};
	VkResult res = context->api.vkGetPhysicalDeviceVideoCapabilitiesKHR(context->physicalDevice, &profileInfo, &capabilities);
	CheckRes(res);
	
	VkFormat supportedDpbFormat;
	VkFormat supportedOutFormat;
	{
		// Get video format properties
		VkVideoProfileListInfoKHR videoProfiles = 
		{ 
			VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR, 
			nullptr, 
			1, 
			&profileInfo,
		};
		VkPhysicalDeviceVideoFormatInfoKHR videoFormatInfo = 
		{
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR,
			&videoProfiles,
			VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR,
		};
		
		u32 supportedFormatCount;
		res = context->api.vkGetPhysicalDeviceVideoFormatPropertiesKHR(context->physicalDevice, &videoFormatInfo, 
																	   &supportedFormatCount, nullptr);
		CheckRes(res);
		Assert(supportedFormatCount > 0);
		
		VkVideoFormatPropertiesKHR formatProperties[16];
		res = context->api.vkGetPhysicalDeviceVideoFormatPropertiesKHR(context->physicalDevice, &videoFormatInfo, 
																	   &supportedFormatCount, formatProperties);
		
		supportedDpbFormat = formatProperties[0].format;
		
		videoFormatInfo.imageUsage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
		res = context->api.vkGetPhysicalDeviceVideoFormatPropertiesKHR(context->physicalDevice, &videoFormatInfo, 
																	   &supportedFormatCount, nullptr);
		CheckRes(res);
		Assert(supportedFormatCount > 0);
		
		res = context->api.vkGetPhysicalDeviceVideoFormatPropertiesKHR(context->physicalDevice, &videoFormatInfo, 
																	   &supportedFormatCount, formatProperties);
		
		supportedOutFormat = formatProperties[0].format;
	}
	
	// Get DpbSlots
	u8 stdLevelIdc = 0;
	u32 maxLumaPS = 0;
	{
		switch(decoder->codecContext->level)
		{
			case H265_LEVEL_IDC_1_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_1_0;
				maxLumaPS = 36864;
			}break;
			case H265_LEVEL_IDC_2_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_2_0;				
				maxLumaPS = 122880;	
			}break;
			case H265_LEVEL_IDC_2_1: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_2_1;
				maxLumaPS = 245760;
			}break;
			case H265_LEVEL_IDC_3_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_3_0;
				maxLumaPS = 552960;	
			}break;
			case H265_LEVEL_IDC_3_1: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_3_1;
				maxLumaPS = 983040;	
			}break;
			case H265_LEVEL_IDC_4_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_4_0;
				maxLumaPS = 2228224;	
			}break;
			case H265_LEVEL_IDC_4_1: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_4_1;
				maxLumaPS = 2228224;	
			}break;
			case H265_LEVEL_IDC_5_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_5_0;
				maxLumaPS = 8912896;	
			}break;
			case H265_LEVEL_IDC_5_1: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_5_1;
				maxLumaPS = 8912896;	
			}break;
			case H265_LEVEL_IDC_5_2: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_5_2;
				maxLumaPS = 8912896;	
			}break;
			case H265_LEVEL_IDC_6_0: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_6_0;
				maxLumaPS = 35651584;	
			}break;
			case H265_LEVEL_IDC_6_1: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_6_1;
				maxLumaPS = 35651584;	
			}break;
			case H265_LEVEL_IDC_6_2: 
			{
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_6_2;
				maxLumaPS = 35651584;	
			}break;
			default: 
			{
				// invalid h.265 idc level
				stdLevelIdc = STD_VIDEO_H265_LEVEL_IDC_6_2;
				maxLumaPS = 35651584;
			}break;
		}
		
		
	}
	
	VkExtent2D maxCodedExtent = {};
	maxCodedExtent.width = 4096;
	maxCodedExtent.height = 4096;
	
	VkVideoSessionCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
	createInfo.pVideoProfile = &profileInfo;
    createInfo.queueFamilyIndex = context->queueDecodeFamily; // Careful, is this main render or secondary render
    createInfo.pictureFormat = supportedOutFormat;
    createInfo.maxCodedExtent = maxCodedExtent;
    createInfo.maxDpbSlots = 16; // seems to be the maxiumum settable
    createInfo.maxActiveReferencePictures = 16;
	createInfo.referencePictureFormat = supportedDpbFormat;
	
	switch(decoder->codec->id)
	{
		case AV_CODEC_ID_H264:
		{
			createInfo.pStdHeaderVersion = &h264DecodeStdExtensionVersion;
		}break;
		case AV_CODEC_ID_H265:
		{
			createInfo.pStdHeaderVersion = &h265DecodeStdExtensionVersion;
		}break;
		default:
		{
			// NOTE(Ecy): impossible code path
			Assert(false);
		}break;
	}
	
	VkResult result = context->api.vkCreateVideoSessionKHR(context->device, &createInfo, nullptr, &session);
	if(result != VK_SUCCESS)
	{
		return result;
	}
	
	u32 memoryRequirementCount = 0;
	VkVideoSessionMemoryRequirementsKHR memoryRequirements[MAX_BOUND_MEMORY] = {};
	
	result = context->api.vkGetVideoSessionMemoryRequirementsKHR(context->device, session, &memoryRequirementCount, nullptr);
	CheckRes(result);
	Assert(memoryRequirementCount <= MAX_BOUND_MEMORY);
	
	result = context->api.vkGetVideoSessionMemoryRequirementsKHR(context->device, session, 
																 &memoryRequirementCount, memoryRequirements);
	CheckRes(result);
	
	// Continue to allocate resources after creating video session: VulkanVideoSession.cpp: l.94
	
	u32 decodeSessionBindMemoryCount = memoryRequirementCount;
	VkBindVideoSessionMemoryInfoKHR decodeSessionBindMemory[MAX_BOUND_MEMORY];
	
	for(u32 index = 0;
		index < decodeSessionBindMemoryCount;
		++index)
	{
		u32 memoryTypeIndex = 0;
        u32 memoryTypeBits = memoryRequirements[index].memoryRequirements.memoryTypeBits;
        if (memoryTypeBits == 0) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }
		
        // Find an available memory type that satisfies the requested properties.
        for (; !(memoryTypeBits & 1); ++memoryTypeIndex ) {
            memoryTypeBits >>= 1;
        }
		
        VkMemoryAllocateInfo memInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                          // sType
            NULL,                                                            // pNext
            memoryRequirements[index].memoryRequirements.size,               // allocationSize
            memoryTypeIndex,                                                 // memoryTypeIndex
        };
		
        result = vkAllocateMemory(context->device, &memInfo, nullptr, &memoryBound[index]);
        if (result != VK_SUCCESS) {
            return result;
        }
		
        Assert(result == VK_SUCCESS);
        decodeSessionBindMemory[index].pNext  = NULL;
        decodeSessionBindMemory[index].sType  = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
        decodeSessionBindMemory[index].memory = memoryBound[index];
		
        decodeSessionBindMemory[index].memoryBindIndex = memoryRequirements[index].memoryBindIndex;
        decodeSessionBindMemory[index].memoryOffset = 0;
        decodeSessionBindMemory[index].memorySize = memoryRequirements[index].memoryRequirements.size;
	}
	
	result = context->api.vkBindVideoSessionMemoryKHR(context->device, session, decodeSessionBindMemoryCount, 
													  decodeSessionBindMemory);
	if(result != VK_SUCCESS)
	{
		return result;
	}
	
	{
		// Define output info
	}
	
	return result;
}
