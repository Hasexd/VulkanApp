#include "VulkanEngine.h"

#define VMA_IMPLEMENTATION
#include <regex>
#include <vk_mem_alloc.h>


namespace
{
	void TransitionImage(const VkCommandBuffer cmd, const VkImage image, const VkImageLayout currentLayout, const VkImageLayout newLayout, uint32_t mipLevels)
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
		subImage.levelCount = mipLevels;
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

		if (fileSize % 4 != 0) 
		{
			std::println("Shader file size not a multiple of 4: {} bytes", fileSize);
			return {};
		}

		std::vector<uint32_t> buffer(fileSize / 4);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		return buffer;
	}

	ShaderName StringToShaderName(const std::string& string)
	{
		if (string == "ray_tracing" || string == "ray_tracing.comp")
			return ShaderName::RAY_TRACING;
		if (string == "bloom" || string == "bloom.comp")
			return ShaderName::BLOOM;
		if(string == "tone_mapping" || string == "tone_mapping.comp")
			return ShaderName::TONE_MAPPING;

		return ShaderName::NONE;
	}

	bool CompileShader(const std::filesystem::path& path)
	{
		const std::string fileName = path.stem().string();
		const std::filesystem::path pathToCompiled = path.parent_path() / "compiled";

		std::string command = std::format("glslang -V \"{}\" -o \"{}/{}.spv\"",
			path.string(), pathToCompiled.string(), fileName);

		std::println("Compiling: {}", command);
		return std::system(command.c_str()) == 0;
	}
}

void VulkanEngine::Init(const std::shared_ptr<GLFWwindow>& window)
{
	m_Window = window;
	m_FileWatcher = FileWatcher(m_ShadersPath, std::chrono::milliseconds(1500));

	InitDevices();
	InitSwapchain();
	InitCommands();
	InitSyncStructures();
	InitImGui();
	InitBuffers();
	InitRenderTargets();
	InitShaders();
	CreateTimestampQueryPool();

	m_FileWatcherFuture = std::async(std::launch::async, [this]() -> void
		{
			MonitorShaders();
		});

	IsInitialized = true;
}

void VulkanEngine::ReloadShaders()
{
	if (!m_ShadersNeedReload)
		return;

	for (const auto& shaderName : m_ChangedShaderFiles)
	{
		const std::filesystem::path shaderPath = m_ShadersPath / shaderName;

		if (CompileShader(shaderPath))
		{
			const std::string& fileName = shaderPath.stem().string();
			std::println("Compiled to: compiled/{}.spv", fileName);

			vkQueueWaitIdle(m_GraphicsQueue);

			const ShaderName shaderName = StringToShaderName(fileName);

			if (shaderName == ShaderName::NONE)
			{
				std::println("Unknown shader name: {}", fileName);
				return;
			}

			const Shader& shader = m_Shaders.at(shaderName);
			const auto& bindings = shader.Bindings;
			shader.Destroy(m_Device);

			CreateShader(shaderName, bindings, std::format("../shaders/compiled/{}.spv", fileName));
			UpdateDescriptorSets(m_Shaders[shaderName]);
		}
		else
		{
			std::println("Compilation failed : {}", m_ShadersPath.stem().string());
		}
	}

	m_ShadersNeedReload = false;
}

void VulkanEngine::MonitorShaders()
{
	m_FileWatcher.Start([this](const std::string& pathToWatch, FileStatus status) -> void 
		{
			if (!std::filesystem::is_regular_file(std::filesystem::path(pathToWatch)) && status != FileStatus::ERASED)
				return;

			const std::filesystem::path shaderPath(pathToWatch);

			if (status != FileStatus::ERASED && shaderPath.extension().string() != ".comp")
				return;

			size_t lastSlash = pathToWatch.find_last_of("/\\");
			std::string fileName = (lastSlash == std::string::npos) ? pathToWatch : pathToWatch.substr(lastSlash + 1);

			switch (status)
			{
			case FileStatus::CREATED:
				std::println("New shader created: {}", fileName);
				break;
			case FileStatus::MODIFIED:
			{
				std::println("Shader modified: {}", fileName);
				m_ChangedShaderFiles.emplace_back(fileName);
				m_ShadersNeedReload = true;
				break;
			}
			case FileStatus::ERASED:
				std::println("Shader erased: {}", fileName);
				break;
			}
	});
}

void VulkanEngine::OnWindowResize(const uint32_t width, const uint32_t height)
{
	vkDeviceWaitIdle(m_Device);
	RecreateSwapchain(width, height);
}

void VulkanEngine::SetViewportSize(const uint32_t width, const uint32_t height)
{
	if (width == m_ViewportWidth && height == m_ViewportHeight)
		return;

	m_ViewportWidth = width;
	m_ViewportHeight = height;

	RecreateRenderTargets();
}


void VulkanEngine::DrawFrame(const bool dispatchCompute)
{
	FrameData& frame = GetCurrentFrame();
	vkWaitForFences(m_Device, 1, &frame.RenderFence, VK_TRUE, UINT64_MAX);
	vkDeviceWaitIdle(m_Device);

	if (m_FrameNumber > 0)
		UpdateTimings();

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
	vkCmdResetQueryPool(cmd, m_TimestampQueryPool, 0, 4);
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_TimestampQueryPool, 2);

	if (dispatchCompute)
	{

		if (m_FrameNumber > 0) 
		{
			TransitionImage(
				cmd,
				m_HDRImage.Image,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				1
			);

			TransitionImage(cmd, m_LDRImage.Image, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 1);
		}

		const Shader& rtShader = m_Shaders.at(ShaderName::RAY_TRACING);
		const Shader& bloomShader = m_Shaders.at(ShaderName::BLOOM);
		const Shader& toneMappingShader = m_Shaders.at(ShaderName::TONE_MAPPING);

		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_TimestampQueryPool, 0);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, rtShader.Pipeline);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			rtShader.PipelineLayout,
			0, 1, &rtShader.DescriptorSet,
			0, nullptr
		);
		uint32_t gx = (m_ViewportWidth + 15) / 16;
		uint32_t gy = (m_ViewportHeight + 15) / 16;
		vkCmdDispatch(cmd, gx, gy, 1);

		//vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		/*vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, bloomShader.Pipeline);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			bloomShader.PipelineLayout,
			0, 1, &bloomShader.DescriptorSet,
			0, nullptr
		);
		vkCmdDispatch(cmd, gx, gy, 1);*/

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, toneMappingShader.Pipeline);
		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			toneMappingShader.PipelineLayout,
			0, 1, &toneMappingShader.DescriptorSet,
			0, nullptr
		);
		vkCmdDispatch(cmd, gx, gy, 1);

		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, m_TimestampQueryPool, 1);

		TransitionImage(
			cmd,
			m_HDRImage.Image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			1
		);

		TransitionImage(
			cmd,
			m_LDRImage.Image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			1
		);
	}

	TransitionImage(
		cmd,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		1
	);

	DrawImGui(cmd, m_SwapchainImageViews[swapchainImageIndex]);

	TransitionImage(
		cmd,
		m_SwapchainImages[swapchainImageIndex],
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		1
	);

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPool, 3);
	vkEndCommandBuffer(cmd);

	VkSemaphoreSubmitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	waitInfo.semaphore = frame.SwapchainSemaphore;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	waitInfo.value = 0;

	VkSemaphoreSubmitInfo signalInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	signalInfo.semaphore = frame.RenderSemaphore;
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
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

void VulkanEngine::RecreateRenderTargets()
{
	vkDeviceWaitIdle(m_Device);

	vkDestroyImageView(m_Device, m_LDRImage.ImageView, nullptr);
	DestroyImage(m_LDRImage);

	vkDestroyImageView(m_Device, m_HDRImage.ImageView, nullptr);
	DestroyImage(m_HDRImage);

	vkDestroyImageView(m_Device, m_AccumulationImage.ImageView, nullptr);
	DestroyImage(m_AccumulationImage);

	InitRenderTargets();

	Shader& rtShader = m_Shaders[ShaderName::RAY_TRACING];
	Shader& bloomShader = m_Shaders[ShaderName::BLOOM];
	Shader& toneMappingShader = m_Shaders[ShaderName::TONE_MAPPING];

	rtShader.Bindings[0] = DescriptorBinding(m_HDRImage);
	rtShader.Bindings[1] = DescriptorBinding(m_AccumulationImage);

	bloomShader.Bindings[0] = DescriptorBinding(m_HDRImage);

	toneMappingShader.Bindings[0] = DescriptorBinding(m_HDRImage);
	toneMappingShader.Bindings[1] = DescriptorBinding(m_LDRImage);

	UpdateDescriptorSets(rtShader);
	UpdateDescriptorSets(bloomShader);
	UpdateDescriptorSets(toneMappingShader);
}

void VulkanEngine::UpdateTimings()
{
	uint64_t timestamps[4];
	VkResult result = vkGetQueryPoolResults(m_Device, m_TimestampQueryPool, 0, 4,
		sizeof(timestamps), timestamps, sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT);

	if (result == VK_SUCCESS)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);

		if (timestamps[1] > timestamps[0]) 
		{ 
			uint64_t rayTracingElapsed = timestamps[1] - timestamps[0];
			float rayTracingMs = rayTracingElapsed * props.limits.timestampPeriod / 1000000.0f;

			m_RenderTime.RayTracingTime = rayTracingMs;
		}
		else
		{
			m_RenderTime.RayTracingTime = 0.0f;
		}

		uint64_t fullFrameElapsed = timestamps[3] - timestamps[2];
		float fullFrameMs = fullFrameElapsed * props.limits.timestampPeriod / 1000000.0f;

		m_RenderTime.FullScreenTime = fullFrameMs;
	}
}

void VulkanEngine::DrawImGui(const VkCommandBuffer cmd, const VkImageView targetImageView) const
{
	VkRenderingAttachmentInfo colorAttachmentInfo{};
	colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO; VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
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

void VulkanEngine::RecreateSwapchain(uint32_t width, uint32_t height)
{
	DestroySwapchain();
	CreateSwapchain(width, height);
}

void VulkanEngine::InitImGui()
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
	poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
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

void VulkanEngine::InitShaders()
{
	const std::vector<DescriptorBinding> rtBindings =
	{
		DescriptorBinding(m_HDRImage),
		DescriptorBinding(m_AccumulationImage),
		DescriptorBinding(UniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
		DescriptorBinding(SphereBuffer),
		DescriptorBinding(MaterialBuffer)
	};

	CreateShader(ShaderName::RAY_TRACING, rtBindings, "../shaders/compiled/ray_tracing.spv");

	const std::vector<DescriptorBinding> bloomBindings =
	{
		DescriptorBinding(m_HDRImage)
	};

	CreateShader(ShaderName::BLOOM, bloomBindings, "../shaders/compiled/bloom.spv");

	const std::vector<DescriptorBinding> toneMappingBindings =
	{
		DescriptorBinding(m_HDRImage),
		DescriptorBinding(m_LDRImage)
	};

	CreateShader(ShaderName::TONE_MAPPING, toneMappingBindings, "../shaders/compiled/tone_mapping.spv");

	const Shader& rtShader = m_Shaders.at(ShaderName::RAY_TRACING);
	const Shader& bloomShader = m_Shaders.at(ShaderName::BLOOM);
	const Shader& toneMappingShader = m_Shaders.at(ShaderName::TONE_MAPPING);

	UpdateDescriptorSets(rtShader);
	UpdateDescriptorSets(bloomShader);
	UpdateDescriptorSets(toneMappingShader);
}

void VulkanEngine::CreateShader(const ShaderName& shaderName,
	const std::vector<DescriptorBinding>& bindings,
	const std::filesystem::path& path)
{
	Shader shader;
	shader.Bindings = bindings;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;

	for (size_t i = 0; i < bindings.size(); ++i)
	{
		layoutBindings.emplace_back(
			static_cast<uint32_t>(i),        
			bindings[i].Type,                
			1,                               
			VK_SHADER_STAGE_COMPUTE_BIT      
		);
		descriptorCounts[bindings[i].Type]++;
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const auto& [type, count] : descriptorCounts)
	{
		poolSizes.emplace_back(type, count);
	}
	
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &shader.DescriptorLayout))
	{
		std::println("Failed to create compute descriptor set layout");
		return;
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &shader.DescriptorPool))
	{
		std::println("Failed to create compute descriptor pool");
		return;
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = shader.DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &shader.DescriptorLayout;

	if (vkAllocateDescriptorSets(m_Device, &allocInfo, &shader.DescriptorSet))
	{
		std::println("Failed to allocate compute descriptor set");
		return;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &shader.DescriptorLayout;

	if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &shader.PipelineLayout))
	{
		std::println("Failed to create compute pipeline layout");
		return;
	}

	const std::vector<uint32_t> buffer = LoadShaderFromFile(path);

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
	pipelineInfo.layout = shader.PipelineLayout;

	if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader.Pipeline))
	{
		std::println("Failed to create compute pipeline");
		vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);
		return;
	}

	vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);

	m_Shaders[shaderName] = shader;
}

void VulkanEngine::UpdateDescriptorSets(const Shader& shader) const
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	for (size_t i = 0; i < shader.Bindings.size(); ++i)
	{
		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = shader.DescriptorSet;
		writeDescriptorSet.dstBinding = static_cast<uint32_t>(i);
		writeDescriptorSet.descriptorType = shader.Bindings[i].Type;
		writeDescriptorSet.descriptorCount = 1;

		if (shader.Bindings[i].Type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
			shader.Bindings[i].Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			writeDescriptorSet.pImageInfo = &shader.Bindings[i].ImageInfo;
		}
		else
		{
			writeDescriptorSet.pBufferInfo = &shader.Bindings[i].BufferInfo;
		}

		descriptorWrites.push_back(writeDescriptorSet);
	}

	vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(), 0, nullptr);
}

void VulkanEngine::InitBuffers()
{
	UniformBuffer = CreateBuffer(sizeof(UniformBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	constexpr size_t maxSpheres = 100;
	SphereBuffer = CreateBuffer(maxSpheres * sizeof(SphereBufferData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	constexpr size_t maxMaterials = 50;
	MaterialBuffer = CreateBuffer(maxMaterials * sizeof(MaterialBufferData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void VulkanEngine::InitRenderTargets()
{
	uint32_t width = m_ViewportWidth, height = m_ViewportHeight;

	if (!width || !height)
	{
		width = 1080;
		height = 720;
	}
	const VkExtent3D imageExtent =  
	{
		width,
		height,
		1
	};

	m_LDRImage = CreateImage(imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	if (m_LDRImage.ImageView == VK_NULL_HANDLE)
	{
		CreateImageView(m_LDRImage, m_LDRImage.ImageFormat);
	}


	m_HDRImage = CreateImage(imageExtent, VK_FORMAT_R16G16B16A16_UNORM, VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	if (m_HDRImage.ImageView == VK_NULL_HANDLE)
	{
		CreateImageView(m_HDRImage, m_HDRImage.ImageFormat);
	}


	m_AccumulationImage = CreateImage(imageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	if (m_AccumulationImage.ImageView == VK_NULL_HANDLE)
	{
		CreateImageView(m_AccumulationImage, m_AccumulationImage.ImageFormat);
	}

	if (m_RenderSampler == VK_NULL_HANDLE) 
	{
		VkSamplerCreateInfo samplerInfo{};

		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_RenderSampler);
	}

	vkResetCommandBuffer(m_ImmediateCommandBuffer, 0);
	VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_ImmediateCommandBuffer, &bi);

	TransitionImage(m_ImmediateCommandBuffer, m_LDRImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	TransitionImage(m_ImmediateCommandBuffer, m_HDRImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	TransitionImage(m_ImmediateCommandBuffer, m_AccumulationImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);

	vkEndCommandBuffer(m_ImmediateCommandBuffer);

	vkResetFences(m_Device, 1, &m_ImmediateFence);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_ImmediateCommandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_ImmediateFence);
	vkQueueWaitIdle(m_GraphicsQueue);

	m_RenderTextureData.SetTexID(reinterpret_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
		m_RenderSampler,
		m_LDRImage.ImageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	)));
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

	for (uint32_t i = 0; i < m_Frames.size(); i++)
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

	for (uint32_t i = 0; i < m_Frames.size(); i++)
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

AllocatedImage VulkanEngine::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage) const
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

	const VkResult result = vmaCreateImage(m_Allocator, &imageInfo, &allocInfo,
		&newImage.Image, &newImage.Allocation, nullptr);

	if (result != VK_SUCCESS)
	{
		std::println("Failed to create vulkan image: {}", static_cast<int>(result));
		return {};
	}

	return newImage;
}


void VulkanEngine::CreateImageView(AllocatedImage& image, const VkFormat format) const
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (format >= VK_FORMAT_D16_UNORM && format <= VK_FORMAT_D32_SFLOAT_S8_UINT)
	{
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (const VkResult result = vkCreateImageView(m_Device, &viewInfo, nullptr, &image.ImageView))
	{
		std::println("Failed to create image view: {}", static_cast<int>(result));
		vmaDestroyImage(m_Allocator, image.Image, image.Allocation);
	}
}

AllocatedBuffer VulkanEngine::CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const
{
	AllocatedBuffer buffer;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memoryUsage;

	if (vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo,
		&buffer.Buffer, &buffer.Allocation, nullptr) != VK_SUCCESS) {
		std::println("Failed to create buffer");
		return {};
	}

	vmaGetAllocationInfo(m_Allocator, buffer.Allocation, &buffer.Info);
	return buffer;
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

	m_Frames.resize(m_SwapchainImages.size());
}

void VulkanEngine::CreateTimestampQueryPool()
{
	VkQueryPoolCreateInfo queryPoolInfo{};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	queryPoolInfo.queryCount = 4;

	vkCreateQueryPool(m_Device, &queryPoolInfo, nullptr, &m_TimestampQueryPool);
}

FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_Frames.size()];
}

void VulkanEngine::DestroyImage(AllocatedImage& image) const
{
	if (image.Image != VK_NULL_HANDLE) 
	{
		vmaDestroyImage(m_Allocator, image.Image, image.Allocation);
		image.Allocation = nullptr;
		image.ImageFormat = VK_FORMAT_UNDEFINED;
	}
}

void VulkanEngine::ResetAccumulation() const
{
	const VkCommandBuffer cmd = m_ImmediateCommandBuffer;
	vkResetCommandBuffer(cmd, 0);

	constexpr VkCommandBufferBeginInfo beginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(cmd, &beginInfo);

	constexpr VkClearColorValue clearColor = { {0.0f, 0.0f, 0.0f, 0.0f } };

	constexpr VkImageSubresourceRange range
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	vkCmdClearColorImage(cmd, m_AccumulationImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);

	vkEndCommandBuffer(cmd);

	vkResetFences(m_Device, 1, &m_ImmediateFence);
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_ImmediateFence);
	vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
}


void VulkanEngine::Cleanup()
{
	if (IsInitialized)
	{
		vkDeviceWaitIdle(m_Device);

		vkDestroySampler(m_Device, m_RenderSampler, nullptr);
		vkDestroyQueryPool(m_Device, m_TimestampQueryPool, nullptr);

		for (const auto& shader : m_Shaders)
		{
			shader.second.Destroy(m_Device);
		}

		for (const auto& frame : m_Frames)
		{
			vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
			vkDestroyFence(m_Device, frame.RenderFence, nullptr);
			vkDestroySemaphore(m_Device, frame.RenderSemaphore, nullptr);
			vkDestroySemaphore(m_Device, frame.SwapchainSemaphore, nullptr);
		}

		vmaDestroyBuffer(m_Allocator, UniformBuffer.Buffer, UniformBuffer.Allocation);
		vmaDestroyBuffer(m_Allocator, SphereBuffer.Buffer, SphereBuffer.Allocation);
		vmaDestroyBuffer(m_Allocator, MaterialBuffer.Buffer, MaterialBuffer.Allocation);

		DestroyImage(m_LDRImage);
		DestroyImage(m_HDRImage);
		DestroyImage(m_AccumulationImage);

		vkDestroyImageView(m_Device, m_LDRImage.ImageView, nullptr);
		vkDestroyImageView(m_Device, m_HDRImage.ImageView, nullptr);
		vkDestroyImageView(m_Device, m_AccumulationImage.ImageView, nullptr);

		m_MainDeletionQueue.Flush();

		DestroySwapchain();

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDescriptorPool(m_Device, m_ImGuiPool, nullptr);

		vkDestroyDevice(m_Device, nullptr);

		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		m_FileWatcher.Stop();
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