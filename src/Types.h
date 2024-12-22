#pragma once

#include <deque>
#include <functional>
#include <ranges>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

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


struct ComputePushConstants
{
	glm::vec4 Data1;
	glm::vec4 Data2;
	glm::vec4 Data3;
	glm::vec4 Data4;
};

struct ComputeEffect
{
	const char* Name;
	VkPipeline Pipeline;
	VkPipelineLayout Layout;

	ComputePushConstants Data;
};


struct AllocatedBuffer
{
	VkBuffer Buffer;
	VmaAllocation Allocation;
	VmaAllocationInfo Info;
};


struct Vertex
{
	float UvX, UvY;
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec4 Color;
};


struct GPUMeshBuffers
{
	AllocatedBuffer IndexBuffer;
	AllocatedBuffer VertexBuffer;
	VkDeviceAddress VertexBufferAddress;
};

struct GPUDrawPushConstants
{
	glm::mat4 WorldMatrix;
	VkDeviceAddress VertexBuffer;
};