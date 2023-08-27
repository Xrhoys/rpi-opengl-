/* date = August 15th 2023 7:00 pm */

#ifndef RENDERER_VULKAN_H
#define RENDERER_VULKAN_H

#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define SEMAPHORE_POOL_SIZE 256
#define SWAP_COMMAND_POOL_SIZE 16
// NOTE(Ecy): do we need a check return instead?
#define CheckRes(name) Assert(name == VK_SUCCESS)

#define VK_KHR_BIND_VIDEO_SESSION_MEMORY(name) VkResult name(                                  \
VkDevice                                    device, \
VkVideoSessionKHR                           videoSession, \
uint32_t                                    bindSessionMemoryInfoCount, \
const VkBindVideoSessionMemoryInfoKHR*      pBindSessionMemoryInfos)
typedef VK_KHR_BIND_VIDEO_SESSION_MEMORY(vk_khr_bind_video_session_memory);

#define VK_GET_PHYSICAL_DEVICE_VIDEO_CAPABILITIES(name) VkResult name(\
VkPhysicalDevice                            physicalDevice,\
const VkVideoProfileInfoKHR*                pVideoProfile,\
VkVideoCapabilitiesKHR*                     pCapabilities)
typedef VK_GET_PHYSICAL_DEVICE_VIDEO_CAPABILITIES(vk_get_physical_device_video_capabilities);

#define VK_GET_PHYSICAL_DEVICE_VIDEO_FORMAT_PROPERTIES(name) VkResult name(                                                   \
VkPhysicalDevice                            physicalDevice,      \
const VkPhysicalDeviceVideoFormatInfoKHR*   pVideoFormatInfo,    \
uint32_t*                                   pVideoFormatPropertyCount,\
VkVideoFormatPropertiesKHR*                 pVideoFormatProperties)
typedef VK_GET_PHYSICAL_DEVICE_VIDEO_FORMAT_PROPERTIES(vk_get_physical_device_video_format_properties);

#define VK_CREATE_VIDEO_SESSION(name) VkResult name(\
VkDevice                                    device,\
const VkVideoSessionCreateInfoKHR*          pCreateInfo,\
const VkAllocationCallbacks*                pAllocator,\
VkVideoSessionKHR*                          pVideoSession)
typedef VK_CREATE_VIDEO_SESSION(vk_create_video_session);

#define VK_GET_VIDEO_SESSION_MEMORY_REQUIREMENTS(name) VkResult name( \
VkDevice                                    device, \
VkVideoSessionKHR                           videoSession, \
uint32_t*                                   pMemoryRequirementsCount, \
VkVideoSessionMemoryRequirementsKHR*        pMemoryRequirements);
typedef VK_GET_VIDEO_SESSION_MEMORY_REQUIREMENTS(vk_get_video_session_memory_requirements);

struct vk_khr_video_interface
{
	vk_khr_bind_video_session_memory                   *vkBindVideoSessionMemoryKHR;
	vk_get_physical_device_video_capabilities          *vkGetPhysicalDeviceVideoCapabilitiesKHR;
	vk_get_physical_device_video_format_properties     *vkGetPhysicalDeviceVideoFormatPropertiesKHR;
	vk_create_video_session                            *vkCreateVideoSessionKHR;
	vk_get_video_session_memory_requirements           *vkGetVideoSessionMemoryRequirementsKHR;
};

struct vk_per_frame
{
	VkFence fence;
	
	VkCommandPool   commandPool;
	VkCommandBuffer commandBuffer;
	VkFramebuffer   framebuffer;
	
	VkImage         backbuffer;
	VkImageView     backbufferView;
};

struct vk_frame_semaphores
{
	VkSemaphore     imageAcquiredSemaphore;
	VkSemaphore     renderCompleteSemaphore;
};

struct semaphore_stack
{
	VkSemaphore pool_[SEMAPHORE_POOL_SIZE];
	u32         count;	
};

struct vk_render_context
{
	vk_khr_video_interface api;
	
	VkInstance            instance;
	VkPhysicalDevice      physicalDevice;
	VkDevice              device;
	VkSurfaceKHR          surface;
	VkSwapchainKHR        swapchain;
	u32                   imageCount;
	VkSemaphore           queueSubmitFence;
	u32                   queueFamily;
	u32                   queueDecodeFamily;
	u32                   queueDecodeNums;
	VkRenderPass          renderpass;
	VkPipelineLayout      pipeLayout;
	VkPipeline            pipeline;
	VkDescriptorSet       descSet;
	VkQueue               queue = VK_NULL_HANDLE;
	VkQueue               decodeQueue[16];
	VkDescriptorPool      descriptorPool = VK_NULL_HANDLE;
	VkSurfaceFormatKHR    surfaceFormat;
	VkExtent2D            extent;
	VkPresentModeKHR      presentMode;
	
	u32                   frameIndex;
	u32                   semaphoreIndex;
	
	b32                   swapChainRebuild;
	
	vk_per_frame          perFrame[16];
	vk_frame_semaphores   semaphores[16];
	semaphore_stack       semaPool;
	
	VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
};

inline VkSemaphore
SemaStackBackAndPop(semaphore_stack *stack)
{
	Assert(stack->count > 0);
	return stack->pool_[--stack->count];
}

inline void
SemaStackPush(semaphore_stack *stack, VkSemaphore *sema)
{
	Assert(stack->count < SEMAPHORE_POOL_SIZE);
	stack->pool_[stack->count++] = *sema;
}

#endif //RENDERER_VULKAN_H
