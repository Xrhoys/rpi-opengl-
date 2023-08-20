/* date = August 15th 2023 7:00 pm */

#ifndef RENDERER_VULKAN_H
#define RENDERER_VULKAN_H

#include <vulkan/vulkan.h>
// TODO(Ecy): include order problem, to fix
#include <windows.h>
#include <vulkan/vulkan_win32.h>

#define SEMAPHORE_POOL_SIZE 256
#define SWAP_COMMAND_POOL_SIZE 16

struct vk_per_frame
{
	VkDevice device;
	VkFence queueSubmitFence;
	VkCommandPool primaryCommandPool;
	VkCommandBuffer primaryCommandBuffer;
	VkSemaphore swapChainSemaphore;
	VkSemaphore swapChainReleaseSemaphore;
	
	i32 queueIndex;
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
	VkImage               images[SWAP_COMMAND_POOL_SIZE];
	VkImageView           backbufferViews[SWAP_COMMAND_POOL_SIZE];
	VkFramebuffer         framebuffer[SWAP_COMMAND_POOL_SIZE];
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
	
	vk_per_frame          perFrame[16];
	semaphore_stack       semaPool;
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
