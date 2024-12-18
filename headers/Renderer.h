#pragma once


#include <vector>
#include <memory>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "RenderData.h"

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
	VkSurfaceKHR m_Surface = nullptr;
	vkb::Device m_Device;
	vkb::DispatchTable m_DispatchTable;
	vkb::Swapchain m_Swapchain;

	std::unique_ptr<RenderData> m_RenderData;
	GLFWwindow* m_Window = nullptr;

	const int MAX_FRAMES_IN_FLIGHT = 2;
};



