#pragma once

#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>


#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vk_mem_alloc.h>

#include "RenderData.h"
#include "Mesh.h"


class VulkanEngine
{

public:
	VulkanEngine();
	~VulkanEngine();
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
	void LoadMeshes();
	void UploadMesh(Mesh& mesh);
	void CreateSurfaceGLFW(VkAllocationCallbacks* allocator = nullptr);
	void Cleanup();
private:
	vkb::Instance m_Instance;
	vkb::InstanceDispatchTable m_InstanceDispatchTable;
	VkSurfaceKHR m_Surface = nullptr;
	vkb::Device m_Device;
	vkb::DispatchTable m_DispatchTable;
	vkb::Swapchain m_Swapchain;
	VmaAllocator m_Allocator;

	std::unique_ptr<RenderData> m_RenderData;
	GLFWwindow* m_Window = nullptr;

	Mesh m_TriangleMesh;

	const int MAX_FRAMES_IN_FLIGHT = 2;

};