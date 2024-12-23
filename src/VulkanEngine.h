#pragma once

#include <vector>
#include <cmath>
#include <filesystem>
#include <functional>
#include <span>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VkBootstrap.h"
#include "VulkanTypes.h"
#include "Image.h"



class VulkanEngine
{
public:
	VulkanEngine() = default;

	void SetWindow(GLFWwindow* window);
	void Init();
	void DrawFrame(const uint32_t* pixelData);
	void OnWindowResize(uint32_t width, uint32_t height);

	void Cleanup();
public:
	bool ResizeRequested = false;
	bool IsInitialized = false;
private:

	void RecreateBuffer();
	void RecreateSwapchain(uint32_t width, uint32_t height);

	void DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView) const;
	void InitVulkan();
	void InitDevices();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitImgui();
	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void DestroyBuffer(const AllocatedBuffer& buffer) const;


	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	FrameData& GetCurrentFrame();
private:
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

	AllocatedBuffer m_Buffer;

	VkFence m_ImmediateFence;
	VkCommandBuffer m_ImmediateCommandBuffer;
	VkCommandPool m_ImmediateCommandPool;

	VkDebugUtilsMessengerEXT m_DebugMessenger;
	GLFWwindow* m_Window;

	VkDescriptorPool m_ImGuiPool;

	DeletionQueue m_MainDeletionQueue;


};