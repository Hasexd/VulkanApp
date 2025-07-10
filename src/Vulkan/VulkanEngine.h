#pragma once

#include <vector>
#include <filesystem>
#include <functional>
#include <span>
#include <array>
#include <print>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VkBootstrap.h"
#include "VulkanTypes.h"

class VulkanEngine
{
public:
	VulkanEngine() = default;

	void SetWindow(const std::shared_ptr<GLFWwindow>& window);
	void Init();
	void DrawFrame(bool dispatchCompute = false);
	void OnWindowResize(uint32_t width, uint32_t height);

	VmaAllocator GetAllocator() const { return m_Allocator; }

	void ResetAccumulation();

	void Cleanup();
public:
	bool IsInitialized = false;
	AllocatedBuffer UniformBuffer;
	AllocatedBuffer SphereBuffer;
	AllocatedBuffer MaterialBuffer;
private:

	AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage) const;
	AllocatedBuffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

	void RecreateSwapchain(uint32_t width, uint32_t height);
	void RecreateRenderTargets();
	void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView) const;
	void InitVulkan();
	void InitDevices();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitImgui();
	void InitComputePipeline();
	void InitRenderTargets();
	void UpdateComputeDescriptorSets() const;
	void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const;

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void DestroyImage(const AllocatedImage& image) const;


	FrameData& GetCurrentFrame();
private:
	std::shared_ptr<GLFWwindow> m_Window;
	VkInstance m_Instance;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface{};

	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	static constexpr uint32_t m_FrameOverlap = 2;
	FrameData m_Frames[m_FrameOverlap];
	uint32_t m_FrameNumber = 0;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily = 0;

	VmaAllocator m_Allocator;

	AllocatedImage m_RenderImage;

	
	AllocatedImage m_AccumulationImage;
	uint32_t m_CurrentSampleCount = 0;

	VkPipelineLayout m_ComputePipelineLayout;
	VkPipeline m_ComputePipeline;
	VkDescriptorSetLayout m_ComputeDescriptorLayout;
	VkDescriptorPool m_ComputeDescriptorPool;
	VkDescriptorSet m_ComputeDescriptorSet;

	VkFence m_ImmediateFence;
	VkCommandBuffer m_ImmediateCommandBuffer;
	VkCommandPool m_ImmediateCommandPool;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkDescriptorPool m_ImGuiPool;

	DeletionQueue m_MainDeletionQueue;
};