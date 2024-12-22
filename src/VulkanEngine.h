#pragma once

#include <vector>
#include <cmath>
#include <filesystem>
#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VkBootstrap.h"
#include "Types.h"
#include "Image.h"
#include "Descriptors.h"
#include "Pipelines.h"




class VulkanEngine
{
public:
	VulkanEngine() = default;

	void SetWindow(GLFWwindow* window);
	void Init();
	void DrawFrame();
	void OnWindowResize(uint32_t width, uint32_t height);
	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	std::vector<ComputeEffect>& GetBackgroundEffects() { return m_BackgroundEffects; }
	int& GetCurrentBackgroundEffects() { return m_CurrentBackgroundEffect; }

	void Cleanup();
public:
	bool IsInitialized = false;
private:
	void DrawBackground(const VkCommandBuffer& cmd);
	void DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView) const;

	void InitVulkan();
	void InitDevices();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitImgui();
	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();

	FrameData& GetCurrentFrame();
private:
	VkInstance m_Instance;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface{};

	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;
	VkExtent2D m_SwapchainExtent;

	static constexpr uint32_t m_FrameOverlap = 2;
	FrameData m_Frames[m_FrameOverlap];
	uint32_t m_FrameNumber = 0;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily = 0;

	VmaAllocator m_Allocator;

	AllocatedImage m_DrawImage;
	VkExtent2D m_DrawExtent;

	DescriptorAllocator m_GlobalDescriptorAllocator;
	
	VkDescriptorSet m_DrawImageDescriptors;
	VkDescriptorSetLayout m_DrawImageDescriptorLayout;

	VkPipeline m_GradientPipeline;
	VkPipelineLayout m_GradientPipelineLayout;

	VkFence m_ImmediateFence;
	VkCommandBuffer m_ImmediateCommandBuffer;
	VkCommandPool m_ImmediateCommandPool;

	std::vector<ComputeEffect> m_BackgroundEffects;
	int m_CurrentBackgroundEffect = 0;

	VkDebugUtilsMessengerEXT m_DebugMessenger;
	GLFWwindow* m_Window;

	DeletionQueue m_MainDeletionQueue;

	bool m_ResizeRequested = false;
};