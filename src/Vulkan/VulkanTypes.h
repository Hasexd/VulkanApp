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

struct AllocatedImage
{
	VkImage Image;
	VkImageView ImageView;
	VmaAllocation Allocation;
	VkExtent3D ImageExtent;
	VkFormat ImageFormat;
};

struct UniformBufferData
{
	alignas(16) glm::vec3 CameraPosition;
	alignas(16) glm::vec3 CameraFrontVector;
	alignas(16) glm::vec3 CameraUpVector;
	alignas(16) glm::vec3 CameraRightVector;
	float AspectRatio;
	float FieldOfView;
	uint32_t SampleCount;
	uint32_t MaxBounces;
	alignas(16) glm::vec3 BackgroundColor;
	uint32_t Width;
	uint32_t Height;
	bool AccumulationEnabled;
};

struct SphereBufferData
{
	alignas(16) glm::vec3 Position;
	float Radius;
	uint32_t MaterialIndex;
};

struct MaterialBufferData
{
	alignas(16) glm::vec3 Color;
	float Roughness;
	float Metallic;
	float EmissionPower;
	alignas(16) glm::vec3 EmissionColor;
};

struct RenderTime
{
	float RayTracingTime = 0.0f;
	float FullScreenTime = 0.0f;
};