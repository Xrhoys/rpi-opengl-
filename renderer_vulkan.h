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
	VkInstance            instance;
	VkPhysicalDevice      physicalDevice;
	VkDevice              device;
	VkSurfaceKHR          surface;
	VkSwapchainKHR        swapchain;
	u32                   imageCount;
	VkSemaphore           queueSubmitFence;
	u32                   queueFamily;
	VkRenderPass          renderpass;
	VkPipelineLayout      pipeLayout;
	VkPipeline            pipeline;
	VkDescriptorSet       descSet;
	VkQueue               queue = VK_NULL_HANDLE;
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
