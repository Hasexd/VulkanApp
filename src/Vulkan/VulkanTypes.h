#pragma once

#include <deque>
#include <functional>
#include <ranges>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>

enum class ShaderName : uint8_t
{
	NONE,
	RAY_TRACING,
	BLOOM,
	TONE_MAPPING
};

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
};

struct RenderTime
{
	float RayTracingTime = 0.0f;
	float FullScreenTime = 0.0f;
};

struct DescriptorBinding
{
	VkDescriptorType Type;

	union
	{
		VkDescriptorImageInfo ImageInfo;
		VkDescriptorBufferInfo BufferInfo;
	};

	explicit DescriptorBinding(const AllocatedImage& image, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		: Type(type)
	{
		ImageInfo.imageView = image.ImageView;
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	explicit DescriptorBinding(const AllocatedBuffer& buffer, const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, const VkDeviceSize size = VK_WHOLE_SIZE)
		: Type(type)
	{
		BufferInfo.buffer = buffer.Buffer;
		BufferInfo.offset = 0;
		BufferInfo.range = size;
	}
};

struct Shader
{
	VkPipelineLayout PipelineLayout;
	VkPipeline Pipeline;
	VkDescriptorSetLayout DescriptorLayout;
	VkDescriptorPool DescriptorPool;
	VkDescriptorSet DescriptorSet;

	std::vector<DescriptorBinding> Bindings;

	void Destroy(const VkDevice& device) const
	{
		vkDestroyPipeline(device, Pipeline, nullptr);
		vkDestroyPipelineLayout(device, PipelineLayout, nullptr);
		vkDestroyDescriptorPool(device, DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, DescriptorLayout, nullptr);
	}
};