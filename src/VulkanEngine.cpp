#include "VulkanEngine.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

static std::filesystem::path s_PathToShaders = "../../shaders/";


void VulkanEngine::OnWindowResize(uint32_t width, uint32_t height)
{
	if (ResizeRequested)
	{
		vkDeviceWaitIdle(m_Device);
		RecreateSwapchain(width, height);
		RecreateBuffer();
		ResizeRequested = false;
	}
}

void VulkanEngine::DrawFrame(const uint32_t* pixelData)
{
	vkWaitForFences(m_Device, 1, &GetCurrentFrame().RenderFence, true, 1000000000);
	GetCurrentFrame().DataDeletionQueue.Flush();
	vkResetFences(m_Device, 1, &GetCurrentFrame().RenderFence);

	uint32_t swapchainImageIndex;

	if (vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, GetCurrentFrame().SwapchainSemaphore, nullptr, &swapchainImageIndex) == VK_ERROR_OUT_OF_DATE_KHR)
	{
		ResizeRequested = true;
	}

	VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;
	vkResetCommandBuffer(cmd, 0);

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &cmdBeginInfo);
	Image::TransitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (pixelData)
	{
		void* data = m_Buffer.Allocation->GetMappedData();
		memcpy(data, pixelData, m_SwapchainExtent.width * m_SwapchainExtent.height * 4);



		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { m_SwapchainExtent.width, m_SwapchainExtent.height, 1 };


		vkCmdCopyBufferToImage(cmd, m_Buffer.Buffer, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	Image::TransitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	DrawImgui(cmd, m_SwapchainImageViews[swapchainImageIndex]);
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

	
	
	if (vkQueuePresentKHR(m_GraphicsQueue, &presentInfo) == VK_ERROR_OUT_OF_DATE_KHR)
	{
		ResizeRequested = true;
	}

	m_FrameNumber++;
}


void VulkanEngine::DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView) const
{
	VkRenderingAttachmentInfo colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachmentInfo.pNext = nullptr;
	colorAttachmentInfo.imageView = targetImageView;
	colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.pNext = nullptr;

	renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, m_SwapchainExtent };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachmentInfo;
	renderInfo.pDepthAttachment = nullptr;
	renderInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);

	
}


void VulkanEngine::Init()
{
	InitVulkan();
	InitImgui();

	uint32_t size = m_SwapchainExtent.width * m_SwapchainExtent.height * 4;

	IsInitialized = true;
}

void VulkanEngine::RecreateBuffer()
{
	uint32_t size = (m_SwapchainExtent.width * m_SwapchainExtent.height * 4);
	vmaDestroyBuffer(m_Allocator, m_Buffer.Buffer, m_Buffer.Allocation);

	m_Buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
}


void VulkanEngine::RecreateSwapchain(uint32_t width, uint32_t height)
{
	DestroySwapchain();
	CreateSwapchain(width, height);
}

void VulkanEngine::InitVulkan()
{
	InitDevices();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
	m_Buffer = CreateBuffer(sizeof(uint32_t) * m_SwapchainExtent.width * m_SwapchainExtent.height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
}

void VulkanEngine::InitImgui()
{
	VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_ImGuiPool);


	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(m_Window, true);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = m_Instance;
	initInfo.PhysicalDevice = m_PhysicalDevice;
	initInfo.Device = m_Device;
	initInfo.QueueFamily = m_GraphicsQueueFamily;
	initInfo.Queue = m_GraphicsQueue;
	initInfo.DescriptorPool = m_ImGuiPool;
	initInfo.MinImageCount = 3;
	initInfo.ImageCount = 3;
	initInfo.UseDynamicRendering = true;
	initInfo.Allocator = m_Allocator->GetAllocationCallbacks();
	initInfo.CheckVkResultFn = [](VkResult err) -> void
	{
		if (err != VK_SUCCESS)
			printf("Error: %d", err);
	};

	initInfo.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;


	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);

	ImGui_ImplVulkan_CreateFontsTexture();

	m_MainDeletionQueue.PushFunction([&]() 
		{
			ImGui_ImplVulkan_Shutdown();
		});
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

	vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_ImmediateFence);

	m_MainDeletionQueue.PushFunction([&]() -> void 
		{
			vkDestroyFence(m_Device, m_ImmediateFence, nullptr);
		});
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

	vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_ImmediateCommandPool);

	VkCommandBufferAllocateInfo cmdAllocInfo{};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;
	cmdAllocInfo.commandPool = m_ImmediateCommandPool;
	cmdAllocInfo.commandBufferCount = 1;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_ImmediateCommandBuffer);

	m_MainDeletionQueue.PushFunction([&]() -> void
	{
		vkDestroyCommandPool(m_Device, m_ImmediateCommandPool, nullptr);
	});
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
	
	m_SwapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkSurfaceFormatKHR surfaceFormat{};
	surfaceFormat.format = m_SwapchainImageFormat;
	surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	vkb::Swapchain vkbSwapchain =
		swapchainBuilder.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		.build()
		.value();

	m_SwapchainExtent = vkbSwapchain.extent;
	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
}


AllocatedBuffer VulkanEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer newBuffer;

	if (vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info) != VK_SUCCESS)
	{
		printf("Error when creating a buffer!");
	}

	if (!ResizeRequested)
	{
		m_MainDeletionQueue.PushFunction([&]() -> void
			{
				vmaDestroyBuffer(m_Allocator, m_Buffer.Buffer, m_Buffer.Allocation);
			});
	}
	
	return newBuffer;
}

void VulkanEngine::DestroyBuffer(const AllocatedBuffer& buffer) const
{
	vmaDestroyBuffer(m_Allocator, buffer.Buffer, buffer.Allocation);
}



FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_FrameOverlap];
}


void VulkanEngine::SetWindow(GLFWwindow* window)
{
	m_Window = window;
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
		vkDestroyDescriptorPool(m_Device, m_ImGuiPool, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		IsInitialized = false;
	}
}

void VulkanEngine::DestroySwapchain()
{
	vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
	for (auto& swapchainImageView : m_SwapchainImageViews)
	{
		vkDestroyImageView(m_Device, swapchainImageView, nullptr);
	}
}

