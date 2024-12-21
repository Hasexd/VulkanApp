#pragma once

#include <vector>
#include <span>
#include <vulkan/vulkan.h>

struct DescriptorLayoutBuilder 
{
	void AddBinding(uint32_t binding, VkDescriptorType type);
	void Clear();
	VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

	std::vector<VkDescriptorSetLayoutBinding> Bindings;
};

struct DescriptorAllocator
{
	struct PoolSizeRatio
	{
		VkDescriptorType Type;
		float Ratio;
	};

	void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
	void ClearDescriptors(VkDevice device);
	void DestroyPool(VkDevice device);

	VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);

	

	VkDescriptorPool Pool;
};