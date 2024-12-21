#pragma once

#include <deque>
#include <functional>
#include <ranges>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>


struct DeletionQueue
{
	void PushFunction(std::function<void()>&& function)
	{
		Deletors.push_back(function);
	}

	void Flush()
	{
		for (const auto& func : Deletors | std::views::reverse)
		{
			func();
		}
	}

	std::deque<std::function<void()>> Deletors;
};


struct FrameData
{
	VkCommandPool CommandPool;
	VkCommandBuffer MainCommandBuffer;

	VkSemaphore SwapchainSemaphore;
	VkSemaphore RenderSemaphore;
	VkFence RenderFence;

	DeletionQueue DeletionQueue;
};



struct AllocatedImage
{
	VkImage Image;
	VkImageView ImageView;
	VmaAllocation Allocation;
	VkExtent3D ImageExtent;
	VkFormat ImageFormat;
};