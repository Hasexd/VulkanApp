#include "Renderer.h"

static std::vector<char> ReadFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if(!file.is_open())
	{
		printf("Failed to open a file!");
		return {};
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

	file.close();

	return buffer;
}


static VkShaderModule CreateShaderModule(Renderer* renderer, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (renderer->GetDispatchTable().createShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

Renderer::Renderer():
	m_RenderData(std::make_unique<RenderData>()) { }

Renderer::~Renderer()
{
	Cleanup();
}


void Renderer::DrawFrame()
{
	m_DispatchTable.waitForFences(1, &m_RenderData->InFlightFences[m_RenderData->CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex = 0;

	VkResult result = m_DispatchTable.acquireNextImageKHR(
		m_Swapchain, UINT64_MAX, m_RenderData->AvailableSemaphores[m_RenderData->CurrentFrame], VK_NULL_HANDLE, &imageIndex
	);

	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		printf("Failed to acquire swapchain image, Error: %d\n", result);
		return;
	}

	if (m_RenderData->ImageInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		m_DispatchTable.waitForFences(1, &m_RenderData->ImageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_RenderData->ImageInFlight[imageIndex] = m_RenderData->InFlightFences[m_RenderData->CurrentFrame];

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_RenderData->AvailableSemaphores[m_RenderData->CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_RenderData->CommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_RenderData->FinishedSemaphore[m_RenderData->CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	m_DispatchTable.resetFences(1, &m_RenderData->InFlightFences[m_RenderData->CurrentFrame]);

	if (m_DispatchTable.queueSubmit(m_RenderData->GraphicsQueue, 1, &submitInfo, m_RenderData->InFlightFences[m_RenderData->CurrentFrame]) != VK_SUCCESS)
	{
		printf("Failed to submit a draw command");
		return;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_Swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = m_DispatchTable.queuePresentKHR(m_RenderData->PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS) {
		printf("failed to present swapchain image\n");
		return;
	}
	m_RenderData->CurrentFrame = (m_RenderData->CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::Init()
{
	InitVulkan();
}


void Renderer::InitVulkan()
{
	InitDevice();
	CreateSwapchain();
	GetQueues();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();
}

void Renderer::RecreateSwapchain()
{
	m_DispatchTable.deviceWaitIdle();
	m_DispatchTable.destroyCommandPool(m_RenderData->CommandPool, nullptr);

	for (auto framebuffer : m_RenderData->Framebuffers)
	{
		m_DispatchTable.destroyFramebuffer(framebuffer, nullptr);
	}

	m_Swapchain.destroy_image_views(m_RenderData->SwapchainImageViews);

	CreateSwapchain();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
}

void Renderer::CreateSyncObjects()
{
	m_RenderData->AvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderData->FinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderData->InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderData->ImageInFlight.resize(m_Swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (m_DispatchTable.createSemaphore(&semaphoreInfo, nullptr, &m_RenderData->AvailableSemaphores[i]) != VK_SUCCESS ||
			m_DispatchTable.createSemaphore(&semaphoreInfo, nullptr, &m_RenderData->FinishedSemaphore[i]) != VK_SUCCESS ||
			m_DispatchTable.createFence(&fenceInfo, nullptr, &m_RenderData->InFlightFences[i]) != VK_SUCCESS) {
			printf("failed to create sync objects\n");
			return;
		}
	}
}

void Renderer::CreateCommandBuffers()
{
	m_RenderData->CommandBuffers.resize(m_RenderData->Framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_RenderData->CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_RenderData->CommandBuffers.size();

	if (m_DispatchTable.allocateCommandBuffers(&allocInfo, m_RenderData->CommandBuffers.data()) != VK_SUCCESS) {
		printf("Failed to allocate a command buffers");
		return;
	}

	for (size_t i = 0; i < m_RenderData->CommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (m_DispatchTable.beginCommandBuffer(m_RenderData->CommandBuffers[i], &begin_info) != VK_SUCCESS) {
			printf("Failed to begin command buffers");
			return; 
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderData->RenderPass;
		renderPassInfo.framebuffer =m_RenderData->Framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_Swapchain.extent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_Swapchain.extent.width;
		viewport.height = (float)m_Swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_Swapchain.extent;

		m_DispatchTable.cmdSetViewport(m_RenderData->CommandBuffers[i], 0, 1, &viewport);
		m_DispatchTable.cmdSetScissor(m_RenderData->CommandBuffers[i], 0, 1, &scissor);

		m_DispatchTable.cmdBeginRenderPass(m_RenderData->CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		m_DispatchTable.cmdBindPipeline(m_RenderData->CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderData->GraphicsPipeline);

		m_DispatchTable.cmdDraw(m_RenderData->CommandBuffers[i], 3, 1, 0, 0);

		m_DispatchTable.cmdEndRenderPass(m_RenderData->CommandBuffers[i]);

		if (m_DispatchTable.endCommandBuffer(m_RenderData->CommandBuffers[i]) != VK_SUCCESS) {
			printf("failed to record command buffer\n");
			return;
		}
	}
}

void Renderer::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_Device.get_queue_index(vkb::QueueType::graphics).value();

	if (m_DispatchTable.createCommandPool(&poolInfo, nullptr, &m_RenderData->CommandPool) != VK_SUCCESS) {
		printf("failed to create command pool\n");
		return;
	}
}

void Renderer::CreateFramebuffers()
{
	m_RenderData->SwapchainImages = m_Swapchain.get_images().value();
	m_RenderData->SwapchainImageViews = m_Swapchain.get_image_views().value();

	m_RenderData->Framebuffers.resize(m_RenderData->SwapchainImageViews.size());

	for(size_t i = 0; i < m_RenderData->SwapchainImageViews.size(); i++)
	{
		VkImageView attachments[] = { m_RenderData->SwapchainImageViews[i] };

		VkFramebufferCreateInfo frameBufferInfo = {};

		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = m_RenderData->RenderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = m_Swapchain.extent.width;
		frameBufferInfo.height = m_Swapchain.extent.height;
		frameBufferInfo.layers = 1;

		if(m_DispatchTable.createFramebuffer(&frameBufferInfo, nullptr, &m_RenderData->Framebuffers[i]) != VK_SUCCESS)
		{
			printf("Failed to create a framebuffer");
		}
	}
}


void Renderer::CreateGraphicsPipeline()
{

	std::filesystem::path pathToShaders = std::filesystem::path(SHADERS_DIR);

	auto vertCode = ReadFile(pathToShaders / "vert.spv");
	auto fragCode = ReadFile(pathToShaders / "frag.spv");

	VkShaderModule vertModule = CreateShaderModule(this, vertCode);
	VkShaderModule fragModule = CreateShaderModule(this, fragCode);

	if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
		printf("failed to create shader module\n");
		return;
	}

	VkPipelineShaderStageCreateInfo vertStageInfo = {};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStageInfo = {};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_Swapchain.extent.width;
	viewport.height = (float)m_Swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_Swapchain.extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (m_DispatchTable.createPipelineLayout(&pipelineLayoutInfo, nullptr, &m_RenderData->PipelineLayout) != VK_SUCCESS) {
		printf("failed to create pipeline layout\n");
		return;
	}

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic_info = {};
	dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamic_info.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shaderStages;
	pipeline_info.pVertexInputState = &vertexInputInfo;
	pipeline_info.pInputAssemblyState = &inputAssembly;
	pipeline_info.pViewportState = &viewportState;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &colorBlending;
	pipeline_info.pDynamicState = &dynamic_info;
	pipeline_info.layout = m_RenderData->PipelineLayout;
	pipeline_info.renderPass = m_RenderData->RenderPass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	if (m_DispatchTable.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_RenderData->GraphicsPipeline) != VK_SUCCESS) {
		std::cout << "failed to create pipline\n";
		return;
	}
	m_DispatchTable.destroyShaderModule(fragModule, nullptr);
	m_DispatchTable.destroyShaderModule(vertModule, nullptr);
}


void Renderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_Swapchain.image_format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;


	if(m_DispatchTable.createRenderPass(&renderPassInfo, nullptr, &m_RenderData->RenderPass) != VK_SUCCESS)
	{
		printf("Failed to create a render pass");
	}
}

void Renderer::GetQueues()
{
	auto graphicsQueue = m_Device.get_queue(vkb::QueueType::graphics);

	if(!graphicsQueue.has_value())
	{
		printf("Failed to get graphics queue: %s\n", graphicsQueue.error().message().c_str());
		return;
	}

	m_RenderData->GraphicsQueue = graphicsQueue.value();

	auto presentQueue = m_Device.get_queue(vkb::QueueType::present);
	if(!presentQueue.has_value())
	{
		printf("Failed to get present queue: %s\n", presentQueue.error().message().c_str());
		return;
	}

	m_RenderData->PresentQueue = presentQueue.value();
}

void Renderer::CreateSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder(m_Device);
	auto swapRet = swapchainBuilder.set_old_swapchain(m_Swapchain).build();

	if(!swapRet)
	{
		printf("%s\n", swapRet.error().message().c_str());
		return;
	}
	vkb::destroy_swapchain(m_Swapchain);
	m_Swapchain = swapRet.value();
}

void Renderer::InitDevice()
{
	if(!m_Window)
	{
		printf("Renderer holds no reference to a window.");
		return;
	}

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder.use_default_debug_messenger().request_validation_layers().build();

	if(!instanceRet)
	{
		printf("%s\n", instanceRet.error().message().c_str());
		return;
	}

	m_Instance = instanceRet.value();
	m_InstanceDispatchTable = m_Instance.make_table();
	
	CreateSurfaceGLFW();

	vkb::PhysicalDeviceSelector physDeviceSelector(m_Instance);
	auto physDeviceRet = physDeviceSelector.set_surface(m_Surface).select();

	if(!physDeviceRet)
	{
		printf("%s\n", physDeviceRet.error().message().c_str());
		return;
	}

	vkb::PhysicalDevice physicalDevice = physDeviceRet.value();

	vkb::DeviceBuilder deviceBuilder(physicalDevice);

	auto deviceRet = deviceBuilder.build();
	if(!deviceRet)
	{
		printf("%s\n", deviceRet.error().message().c_str());
		return;
	}

	m_Device = deviceRet.value();
	m_DispatchTable = m_Device.make_table();
}

void Renderer::CreateSurfaceGLFW(VkAllocationCallbacks* allocator /* = nullptr */) 
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult err = glfwCreateWindowSurface(m_Instance, m_Window, allocator, &surface);

	if(err)
	{
		const char* errorMessage;
		int ret = glfwGetError(&errorMessage);
		if(ret != 0)
		{
			printf("%d ", ret);

			if(errorMessage != nullptr)
			{
				printf("%s\n", errorMessage);
			}
		}
		surface = VK_NULL_HANDLE;
	}

	m_Surface = std::move(surface);
}


vkb::DispatchTable& Renderer::GetDispatchTable() 
{
	return m_DispatchTable;
}

void Renderer::SetWindow(GLFWwindow* window)
{
	m_Window = window;
}


void Renderer::Cleanup()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_DispatchTable.destroySemaphore(m_RenderData->FinishedSemaphore[i], nullptr);
		m_DispatchTable.destroySemaphore(m_RenderData->AvailableSemaphores[i], nullptr);
		m_DispatchTable.destroyFence(m_RenderData->InFlightFences[i], nullptr);
	}

	m_DispatchTable.destroyCommandPool(m_RenderData->CommandPool, nullptr);

	for (auto framebuffer : m_RenderData->Framebuffers) {
		m_DispatchTable.destroyFramebuffer(framebuffer, nullptr);
	}

	m_DispatchTable.destroyPipeline(m_RenderData->GraphicsPipeline, nullptr);
	m_DispatchTable.destroyPipelineLayout(m_RenderData->PipelineLayout, nullptr);
	m_DispatchTable.destroyRenderPass(m_RenderData->RenderPass, nullptr);

	m_Swapchain.destroy_image_views(m_RenderData->SwapchainImageViews);

	vkb::destroy_swapchain(m_Swapchain);
	vkb::destroy_device(m_Device);
	vkb::destroy_surface(m_Instance, m_Surface);
	vkb::destroy_instance(m_Instance);
}