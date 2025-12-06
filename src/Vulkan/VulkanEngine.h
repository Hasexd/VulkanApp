#pragma once

#include <vector>
#include <filesystem>
#include <functional>
#include <span>
#include <array>
#include <print>
#include <fstream>
#include <future>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <glm.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VkBootstrap.h"
#include "VulkanTypes.h"
#include "../FileWatcher.h"


class VulkanEngine
{
public:
	VulkanEngine() = default;

	void Init(const std::shared_ptr<GLFWwindow>& window);
	void DrawFrame(bool dispatchCompute = false);
	void OnWindowResize(uint32_t width, uint32_t height);
	void SetViewportSize(uint32_t width, uint32_t height);
	void ReloadShaders();

	[[nodiscard]] ImTextureID GetRenderTextureID() const { return m_RenderTextureData.GetTexID(); }

	[[nodiscard]] VmaAllocator GetAllocator() const { return m_Allocator; }
	[[nodiscard]] const RenderTime& GetRenderTime() const { return m_RenderTime; }

	void ResetAccumulation() const;

	void Cleanup();
public:
	bool IsInitialized = false;
	AllocatedBuffer UniformBuffer;
	AllocatedBuffer SphereBuffer;
	AllocatedBuffer MaterialBuffer;

private:

	[[nodiscard]] AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage) const;
	void CreateImageView(AllocatedImage& image, const VkFormat format) const;
	[[nodiscard]] AllocatedBuffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

	void RecreateSwapchain(uint32_t width, uint32_t height);
	void RecreateRenderTargets();
	void DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView) const;
	void InitDevices();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitImGui();
	void InitShaders();

	void InitRenderTargets();
	void InitBuffers();

	void UpdateDescriptorSets(const Shader& shader) const;

	void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const;

	void CreateTimestampQueryPool();
	void UpdateTimings();

	void CreateShader(const ShaderName& shaderName,
		const std::vector<DescriptorBinding>& bindings,
		const std::filesystem::path& path);

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
	void DestroyImage(AllocatedImage& image) const;

	void MonitorShaders();

	FrameData& GetCurrentFrame();
private:
	std::shared_ptr<GLFWwindow> m_Window;
	VkInstance m_Instance;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	VkSurfaceKHR m_Surface{};

	uint32_t m_ViewportWidth = 0;
	uint32_t m_ViewportHeight = 0;

	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	std::vector<FrameData> m_Frames;
	uint32_t m_FrameNumber = 0;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily = 0;

	VmaAllocator m_Allocator;

	AllocatedImage m_LDRImage;
	AllocatedImage m_HDRImage;
	AllocatedImage m_AccumulationImage;

	VkFence m_ImmediateFence;
	VkCommandBuffer m_ImmediateCommandBuffer;
	VkCommandPool m_ImmediateCommandPool;

	std::unordered_map<ShaderName, Shader> m_Shaders;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkDescriptorPool m_ImGuiPool;
	ImTextureData m_RenderTextureData;
	VkSampler m_RenderSampler;

	RenderTime m_RenderTime;
	VkQueryPool m_TimestampQueryPool;

	DeletionQueue m_MainDeletionQueue;

	FileWatcher m_FileWatcher;
	std::future<void> m_FileWatcherFuture;

	std::vector<std::string> m_ChangedShaderFiles;
	std::atomic<bool> m_ShadersNeedReload = false;
	std::filesystem::path m_ShadersPath = std::filesystem::current_path().parent_path() / "shaders";
};