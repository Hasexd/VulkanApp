#pragma once

#include <deque>
#include <functional>
#include <ranges>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>

struct DeletionQueue
{
	void PushFunction(std::function<void()>&& function)
	{
		Deletors.push_back(std::move(function));
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

	DeletionQueue DataDeletionQueue;
};


struct AllocatedBuffer
{
	VkBuffer Buffer;
	VmaAllocation Allocation;
	VmaAllocationInfo Info;
};
