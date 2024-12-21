#include "VulkanEngine.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>


void VulkanEngine::OnWindowResize()
{
	m_ResizeRequested = true;

	vkDeviceWaitIdle(m_Device);
	DestroySwapchain();
	InitSwapchain();

	m_ResizeRequested = false;
}

void VulkanEngine::DrawFrame()
{
	vkWaitForFences(m_Device, 1, &GetCurrentFrame().RenderFence, true, 1000000000);
	GetCurrentFrame().DeletionQueue.Flush();
	vkResetFences(m_Device, 1, &GetCurrentFrame().RenderFence);

	uint32_t swapchainImageIndex;

	vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, GetCurrentFrame().SwapchainSemaphore, nullptr, &swapchainImageIndex);

	VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
	vkResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &cmdBeginInfo);
	Image::TransitionImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(cmd);

	Image::TransitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Image::CopyImageToImage(cmd, m_DrawImage.Image, m_SwapchainImages[swapchainImageIndex], m_DrawExtent, m_SwapchainExtent);
	Image::TransitionImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	Image::TransitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


	vkEndCommandBuffer(cmd);

	VkCommandBufferSubmitInfo cmdSubmitInfo{};
	cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmdSubmitInfo.pNext = nullptr;
	cmdSubmitInfo.commandBuffer = cmd;
	cmdSubmitInfo.deviceMask = 0;

	VkSemaphoreSubmitInfo waitSubmitInfo{};
	waitSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitSubmitInfo.pNext = nullptr;
	waitSubmitInfo.semaphore = GetCurrentFrame().SwapchainSemaphore;
	waitSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	waitSubmitInfo.deviceIndex = 0;
	waitSubmitInfo.value = 1;

	VkSemaphoreSubmitInfo signalSubmitInfo{};
	signalSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalSubmitInfo.pNext = nullptr;
	signalSubmitInfo.semaphore = GetCurrentFrame().RenderSemaphore;
	signalSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	signalSubmitInfo.deviceIndex = 0;
	signalSubmitInfo.value = 1;


	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.pNext = nullptr;

	submitInfo.waitSemaphoreInfoCount = &waitSubmitInfo == nullptr ? 0 : 1;
	submitInfo.pWaitSemaphoreInfos = &waitSubmitInfo;

	submitInfo.signalSemaphoreInfoCount = &signalSubmitInfo == nullptr ? 0 : 1;
	submitInfo.pSignalSemaphoreInfos = &signalSubmitInfo;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdSubmitInfo;

	vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, GetCurrentFrame().RenderFence);


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &GetCurrentFrame().RenderSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &swapchainImageIndex;

	
	vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);

	m_FrameNumber++;
}

void VulkanEngine::DrawBackground(const VkCommandBuffer& cmd)
{
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(m_FrameNumber / 120.f));
	clearValue = { {0.f, 0.f, flash, 0.f} };

	VkImageSubresourceRange clearRange{};
	clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearRange.baseMipLevel = 0;
	clearRange.levelCount = VK_REMAINING_MIP_LEVELS;
	clearRange.baseArrayLayer = 0;
	clearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	vkCmdClearColorImage(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

}


void VulkanEngine::Init()
{
	InitVulkan();
	IsInitialized = true;
}

void VulkanEngine::InitVulkan()
{
	InitDevices();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
}

void VulkanEngine::InitSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (uint32_t i = 0; i < m_FrameOverlap; i++)
	{
		vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence);
		vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore);
		vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore);
	}

}

void VulkanEngine::InitCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

	for (uint32_t i = 0; i < m_FrameOverlap; i++)
	{
		vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool);

		VkCommandBufferAllocateInfo cmdAllocInfo{};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = m_Frames[i].CommandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].MainCommandBuffer);
	}
}

void VulkanEngine::InitSwapchain()
{
	if (m_Window)
	{
		int width, height;
		glfwGetWindowSize(m_Window, &width, &height);
		CreateSwapchain(width, height);
	}
	else
	{
		printf("Failed to create a swapchain, no window set");
		return;
	}

}

void VulkanEngine::InitDevices()
{
	vkb::InstanceBuilder builder{};

	auto instRet = builder.set_app_name("Vulkan App")
		.request_validation_layers(true)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 2)
		.build();

	vkb::Instance vkbInstance = instRet.value();

	m_Instance = vkbInstance.instance;
	m_DebugMessenger = vkbInstance.debug_messenger;

	glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);

	VkPhysicalDeviceVulkan13Features features{};

	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;


	vkb::PhysicalDeviceSelector selector(vkbInstance);
	vkb::PhysicalDevice physicalDevice =
		selector.set_minimum_version(1, 3)
		.set_surface(m_Surface)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.select()
		.value();


	vkb::DeviceBuilder deviceBuilder(physicalDevice);
	vkb::Device vkbDevice = deviceBuilder.build().value();

	m_Device = vkbDevice.device;
	m_PhysicalDevice = physicalDevice.physical_device;

	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
	allocatorCreateInfo.device = m_Device;
	allocatorCreateInfo.instance = m_Instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	
	vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);

	m_MainDeletionQueue.PushFunction([&]() -> void
		{
			vmaDestroyAllocator(m_Allocator);
		});

}


void VulkanEngine::CreateSwapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder(m_PhysicalDevice, m_Device, m_Surface);
	
	m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkSurfaceFormatKHR surfaceFormat{};
	surfaceFormat.format = m_SwapchainImageFormat;
	surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	vkb::Swapchain vkbSwapchain =
		swapchainBuilder.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_SwapchainExtent = vkbSwapchain.extent;
	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();


	if (!m_ResizeRequested)
	{
		VkExtent3D drawImageExtent{};
		drawImageExtent.width = width;
		drawImageExtent.height = height;
		drawImageExtent.depth = 1;

		m_DrawImage.ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		m_DrawImage.ImageExtent = drawImageExtent;
		m_DrawExtent = { drawImageExtent.width, drawImageExtent.height };

		VkImageUsageFlags drawImageUsageFlags{};
		drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo imageCreateInfo{};

		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = m_DrawImage.ImageFormat;
		imageCreateInfo.extent = m_DrawImage.ImageExtent;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		//change to linear for cpu 
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

		imageCreateInfo.usage = drawImageUsageFlags;

		VmaAllocationCreateInfo imageAllocInfo{};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		vmaCreateImage(m_Allocator, &imageCreateInfo, &imageAllocInfo, &m_DrawImage.Image, &m_DrawImage.Allocation, nullptr);

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.pNext = nullptr;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.image = m_DrawImage.Image;
		imageViewInfo.format = m_DrawImage.ImageFormat;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;



		vkCreateImageView(m_Device, &imageViewInfo, nullptr, &m_DrawImage.ImageView);


		m_MainDeletionQueue.PushFunction([&]() -> void
			{
				vkDestroyImageView(m_Device, m_DrawImage.ImageView, nullptr);
				vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);
			});
	}
}



void VulkanEngine::Cleanup()
{
	if (IsInitialized)
	{
		vkDeviceWaitIdle(m_Device);


		for (uint32_t i = 0; i < m_FrameOverlap; i++)
		{
			vkDestroyCommandPool(m_Device, m_Frames[i].CommandPool, nullptr);
			vkDestroyFence(m_Device, m_Frames[i].RenderFence, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device, m_Frames[i].SwapchainSemaphore, nullptr);
		}
		m_MainDeletionQueue.Flush();


		DestroySwapchain();
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		IsInitialized = false;
	}
}

void VulkanEngine::DestroySwapchain()
{
	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

	for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
	{
		vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
	}
}

FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_FrameOverlap];
}


void VulkanEngine::SetWindow(GLFWwindow* window)
{
	m_Window = window;
}
