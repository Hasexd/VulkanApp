#include "VulkanEngine.h"

#define VMA_IMPLEMENTATION
#include <regex>
#include <vk_mem_alloc.h>


namespace
{
	void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.pNext = nullptr;

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = currentLayout;
		imageBarrier.newLayout = newLayout;

		VkImageSubresourceRange subImage{};
		subImage.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subImage.baseMipLevel = 0;
		subImage.levelCount = 1;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = 1;
		imageBarrier.subresourceRange = subImage;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	

	std::vector<uint32_t> LoadShaderFromFile(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);

		auto const fileSize = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		if (fileSize % 4 != 0) {
			std::println("Shader file size not a multiple of 4: {} bytes", fileSize);
			return {};
		}

		std::vector<uint32_t> buffer(fileSize / 4);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		return buffer;
	}
}

void VulkanEngine::OnWindowResize(uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(m_Device);
	RecreateSwapchain(width, height);
	RecreateRenderTargets(width, height);
}

void VulkanEngine::DrawFrame()
{
	FrameData& frame = GetCurrentFrame();
	vkWaitForFences(m_Device, 1, &frame.RenderFence, VK_TRUE, UINT64_MAX);
	frame.DataDeletionQueue.Flush();
	vkResetFences(m_Device, 1, &frame.RenderFence);

	uint32_t swapchainImageIndex = 0;
	vkAcquireNextImageKHR(
		m_Device,
		m_Swapchain,
		UINT64_MAX,
		frame.SwapchainSemaphore,
		VK_NULL_HANDLE,
		&swapchainImageIndex
	);

	VkCommandBuffer cmd = frame.MainCommandBuffer;
	vkResetCommandBuffer(cmd, 0);
	VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &bi);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_ComputePipelineLayout,
		0, 1, &m_ComputeDescriptorSet,
		0, nullptr
	);
	uint32_t gx = (m_RenderImage.ImageExtent.width + 7) / 8;
	uint32_t gy = (m_RenderImage.ImageExtent.height + 7) / 8;
	vkCmdDispatch(cmd, gx, gy, 1);

	VkImageMemoryBarrier toSrcBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	toSrcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	toSrcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	toSrcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	toSrcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	toSrcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toSrcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toSrcBarrier.image = m_RenderImage.Image;
	toSrcBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 };

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr,
		0, nullptr,
		1, &toSrcBarrier
	);

	TransitionImage(
		cmd,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	VkImageBlit blit{};
	blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = {
		int32_t(m_RenderImage.ImageExtent.width),
		int32_t(m_RenderImage.ImageExtent.height),
		1
	};
	blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = {
		int32_t(m_SwapchainExtent.width),
		int32_t(m_SwapchainExtent.height),
		1
	};

	vkCmdBlitImage(
		cmd,
		m_RenderImage.Image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &blit,
		VK_FILTER_LINEAR
	);

	TransitionImage(
		cmd,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	TransitionImage(
		cmd,
		m_RenderImage.Image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL
	);

	DrawImGui(cmd, m_SwapchainImageViews[swapchainImageIndex]);

	TransitionImage(
		cmd,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	vkEndCommandBuffer(cmd);

	VkSemaphoreSubmitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	waitInfo.semaphore = frame.SwapchainSemaphore;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
	waitInfo.value = 0;

	VkSemaphoreSubmitInfo signalInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	signalInfo.semaphore = frame.RenderSemaphore;
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR;
	signalInfo.value = 0;

	VkCommandBufferSubmitInfo cmdInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdInfo.commandBuffer = cmd;

	VkSubmitInfo2 submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &waitInfo;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdInfo;
	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalInfo;

	vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, frame.RenderFence);

	VkPresentInfoKHR pinfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	pinfo.waitSemaphoreCount = 1;
	pinfo.pWaitSemaphores = &frame.RenderSemaphore;
	pinfo.swapchainCount = 1;
	pinfo.pSwapchains = &m_Swapchain;
	pinfo.pImageIndices = &swapchainImageIndex;
	vkQueuePresentKHR(m_GraphicsQueue, &pinfo);

	m_FrameNumber++;
}


void VulkanEngine::RecreateRenderTargets(uint32_t width, uint32_t height)
{
	vkDeviceWaitIdle(m_Device);

	if (m_RenderImage.Image != VK_NULL_HANDLE)
		DestroyImage(m_RenderImage);

	VkExtent3D imageExtent = {
		.width = width,
		.height = height,
		.depth = 1
	};

	m_RenderImage = CreateImage(imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_RenderImage.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_RenderImage.ImageFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_RenderImage.ImageView))
	{
		std::println("Failed to create image view for render target");
		return;
	}

	TransitionImageLayout(m_RenderImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	UpdateComputeDescriptorSets();
}

void VulkanEngine::UpdateComputeDescriptorSets() const
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = m_RenderImage.ImageView;
	imageInfo.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_ComputeDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
}



void VulkanEngine::DrawImGui(VkCommandBuffer cmd, VkImageView targetImageView) const
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
	InitComputePipeline();
	InitRenderTargets();
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
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(m_Window.get(), true);

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
			std::println("Error: {}", (int)err);
	};

	initInfo.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;


	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);

	m_MainDeletionQueue.PushFunction([&]() 
		{
			ImGui_ImplVulkan_Shutdown();
		});
}

void VulkanEngine::InitComputePipeline()
{
	VkDescriptorSetLayoutBinding imageBinding{};
	imageBinding.binding = 0;
	imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageBinding.descriptorCount = 1;
	imageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &imageBinding;

	if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_ComputeDescriptorLayout))
	{
		std::println("Failed to create compute descriptor set layout");
		return;
	}

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize.descriptorCount = 10;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_ComputeDescriptorPool))
	{
		std::println("Failed to create compute descriptor pool");
		return;
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_ComputeDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_ComputeDescriptorLayout;

	if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_ComputeDescriptorSet))
	{
		std::println("Failed to allocate compute descriptor set");
		return;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_ComputeDescriptorLayout;

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_ComputePipelineLayout))
	{
		std::println("Failed to create compute pipeline layout");
		return;
	}
	const std::vector<uint32_t> buffer = LoadShaderFromFile("../shaders/computeShader.spv");

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule computeShaderModule;
	if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &computeShaderModule))
	{
		std::println("Failed to create compute shader module");
		return;
	}

	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = computeShaderModule;
	shaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageInfo;
	pipelineInfo.layout = m_ComputePipelineLayout;

	if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_ComputePipeline))
	{
		std::println("Failed to create compute pipeline");
		vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);
		return;
	}

	vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);

	m_MainDeletionQueue.PushFunction([*this] {
		vkDestroyPipeline(m_Device, m_ComputePipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_ComputePipelineLayout, nullptr);
		vkDestroyDescriptorPool(m_Device, m_ComputeDescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_ComputeDescriptorLayout, nullptr);
	});
}


void VulkanEngine::InitRenderTargets()
{
	int width, height;
	glfwGetFramebufferSize(m_Window.get(), &width, &height);

	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	m_RenderImage = CreateImage(imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_RenderImage.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_RenderImage.ImageFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_RenderImage.ImageView))
	{
		std::println("Failed to create image view for render target");
		return;
	}

	TransitionImageLayout(m_RenderImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	UpdateComputeDescriptorSets();

	m_MainDeletionQueue.PushFunction([=]() {
		DestroyImage(m_RenderImage);
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
		glfwGetWindowSize(m_Window.get(), &width, &height);
		CreateSwapchain(width, height);
	}
	else
	{
		std::println("Failed to create a swapchain, no window set");
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

	glfwCreateWindowSurface(m_Instance, m_Window.get(), nullptr, &m_Surface);

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

AllocatedImage VulkanEngine::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage)
{
	AllocatedImage newImage = {};
	newImage.ImageFormat = format;
	newImage.ImageExtent = size;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = size;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (vmaCreateImage(m_Allocator, &imageInfo, &allocInfo,
		&newImage.Image, &newImage.Allocation, nullptr))
	{
		std::println("Failed to create a vulkan image");
		return {};
	}

	return newImage;
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

FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_FrameOverlap];
}


void VulkanEngine::SetWindow(const std::shared_ptr<GLFWwindow>& window)
{
	m_Window = window;
}

void VulkanEngine::DestroyImage(const AllocatedImage& image) const
{
	if (image.ImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_Device, image.ImageView, nullptr);
	}
	if (image.Image != VK_NULL_HANDLE) {
		vmaDestroyImage(m_Allocator, image.Image, image.Allocation);
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

void VulkanEngine::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
	VkCommandBuffer cmd = m_ImmediateCommandBuffer;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult result = vkBeginCommandBuffer(cmd, &beginInfo);
	if (result != VK_SUCCESS) {
		std::println("Failed to begin command buffer for image transition");
		return;
	}

	VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imageBarrier.pNext = nullptr;
	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;

	VkImageSubresourceRange subImage{};
	subImage.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subImage.baseMipLevel = 0;
	subImage.levelCount = 1;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = 1;
	imageBarrier.subresourceRange = subImage;
	imageBarrier.image = image;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(cmd, &depInfo);

	result = vkEndCommandBuffer(cmd);
	if (result != VK_SUCCESS) {
		std::println("Failed to end command buffer for image transition");
		return;
	}

	vkResetFences(m_Device, 1, &m_ImmediateFence);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_ImmediateFence);
	if (result != VK_SUCCESS) {
		std::println("Failed to submit command buffer for image transition");
		return;
	}

	result = vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
	if (result != VK_SUCCESS) {
		std::println("Failed to wait for fence in image transition");
		return;
	}

	result = vkResetFences(m_Device, 1, &m_ImmediateFence);
	if (result != VK_SUCCESS) {
		std::println("Failed to reset fence in image transition");
		return;
	}

	result = vkResetCommandBuffer(cmd, 0);
	if (result != VK_SUCCESS) {
		std::println("Failed to reset command buffer in image transition");
		return;
	}
}