#pragma once


#include <vector>
#include <memory>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <filesystem>
#include <fstream>
#include <iostream>

struct RenderData;

class Renderer
{
public:
	Renderer();
	~Renderer();


	void DrawFrame();

	void SetWindow(GLFWwindow* window);
	void Init();
	vkb::DispatchTable& GetDispatchTable();

private:
	void InitVulkan();
	void InitDevice();
	void CreateSwapchain();
	void GetQueues();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void RecreateSwapchain();
	void CreateSurfaceGLFW(VkAllocationCallbacks* allocator = nullptr);


	void Cleanup();
private:
	vkb::Instance m_Instance;
	vkb::InstanceDispatchTable m_InstanceDispatchTable;
	VkSurfaceKHR m_Surface;
	vkb::Device m_Device;
	vkb::DispatchTable m_DispatchTable;
	vkb::Swapchain m_Swapchain;

	std::unique_ptr<RenderData> m_RenderData;
	GLFWwindow* m_Window;

	const int MAX_FRAMES_IN_FLIGHT = 2;
};



struct RenderData
{
	VkQueue GraphicsQueue;
	VkQueue PresentQueue;

	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;
	std::vector<VkFramebuffer> Framebuffers;

	VkRenderPass RenderPass;
	VkPipelineLayout PipelineLayout;
	VkPipeline GraphicsPipeline;

	VkCommandPool CommandPool;
	std::vector<VkCommandBuffer> CommandBuffers;

	std::vector<VkSemaphore> AvailableSemaphores;
	std::vector<VkSemaphore> FinishedSemaphore;
	std::vector<VkFence> InFlightFences;
	std::vector<VkFence> ImageInFlight;

	size_t CurrentFrame = 0;
};