#pragma once

#include <vector>
#include <cmath>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkBootstrap.h"

struct FrameData
{
	VkCommandPool CommandPool;
	VkCommandBuffer MainCommandBuffer;

	VkSemaphore SwapchainSemaphore;
	VkSemaphore RenderSemaphore;
	VkFence RenderFence;
};

class VulkanEngine
{
public:
	VulkanEngine() = default;

	void SetWindow(GLFWwindow* window);
	void Init();
	void DrawFrame();

	void Cleanup();
public:
	bool IsInitialized = false;
private:
	void InitVulkan();
	void InitDevices();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
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

	static const uint32_t m_FrameOverlap = 2;
	FrameData m_Frames[m_FrameOverlap];
	uint32_t m_FrameNumber = 0;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily = 0;

	VkDebugUtilsMessengerEXT m_DebugMessenger;
	GLFWwindow* m_Window;

};