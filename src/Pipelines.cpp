
#include "Pipelines.h"

void PipelineBuilder::Clear()
{

	m_InputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	m_Rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	m_ColorBlendAttachment = {};

	m_Multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	m_PipelineLayout = {};

	m_DepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	m_RenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	m_ShaderStages.clear();
}

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &m_ColorBlendAttachment;

	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };


	VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext = &m_RenderInfo;
	pipelineInfo.stageCount = (uint32_t)m_ShaderStages.size();
	pipelineInfo.pStages = m_ShaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_InputAssembly;;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &m_Rasterizer;
	pipelineInfo.pMultisampleState = &m_Multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &m_DepthStencil;
	pipelineInfo.layout = m_PipelineLayout;
	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	pipelineInfo.pDynamicState = &dynamicInfo;
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
		nullptr, &newPipeline)
		!= VK_SUCCESS) {
		printf("failed to create pipeline");
		return VK_NULL_HANDLE;
	}
	else {
		return newPipeline;
	}

}

void PipelineBuilder::SetShaders(VkShaderModule vertexShader, const char* vertName,  VkShaderModule fragmentShader, const char* fragName)
{
	m_ShaderStages.clear();

	VkPipelineShaderStageCreateInfo vertexInfo{};
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexInfo.pNext = nullptr;
	vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexInfo.module = vertexShader;
	vertexInfo.pName = vertName;

	m_ShaderStages.push_back(vertexInfo);

	VkPipelineShaderStageCreateInfo fragmentInfo{};
	fragmentInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentInfo.pNext = nullptr;
	fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentInfo.module = fragmentShader;
	fragmentInfo.pName = fragName;

	m_ShaderStages.push_back(fragmentInfo);
}
void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
{
	m_InputAssembly.topology = topology;
	m_InputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
	m_Rasterizer.polygonMode = mode;
	m_Rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	m_Rasterizer.cullMode = cullMode;
	m_Rasterizer.frontFace = frontFace;
}

void PipelineBuilder::SetMultisamplingNone()
{
	m_Multisampling.sampleShadingEnable = VK_FALSE;
	m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_Multisampling.minSampleShading = 1.0f;
	m_Multisampling.pSampleMask = nullptr;
	m_Multisampling.alphaToCoverageEnable = VK_FALSE;
	m_Multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::DisableBlending()
{
	m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_ColorBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::EnableBlendingAdditive()
{
	m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_ColorBlendAttachment.blendEnable = VK_TRUE;
	m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::EnableBlendingAlphablend()
{
	m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_ColorBlendAttachment.blendEnable = VK_TRUE;
	m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
{
	m_ColorAttachmentformat = format;
	m_RenderInfo.colorAttachmentCount = 1;
	m_RenderInfo.pColorAttachmentFormats = &m_ColorAttachmentformat;
}

void PipelineBuilder::SetDepthFormat(VkFormat format)
{
	m_RenderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::DisableDepthtest()
{
	m_DepthStencil.depthTestEnable = VK_FALSE;
	m_DepthStencil.depthWriteEnable = VK_FALSE;
	m_DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
	m_DepthStencil.stencilTestEnable = VK_FALSE;
	m_DepthStencil.front = {};
	m_DepthStencil.back = {};
	m_DepthStencil.minDepthBounds = 0.f;
	m_DepthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::EnableDepthtest(bool depthWriteEnable, VkCompareOp op)
{
	m_DepthStencil.depthTestEnable = VK_TRUE;
	m_DepthStencil.depthWriteEnable = depthWriteEnable;
	m_DepthStencil.depthCompareOp = op;
	m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
	m_DepthStencil.stencilTestEnable = VK_FALSE;
	m_DepthStencil.front = {};
	m_DepthStencil.back = {};
	m_DepthStencil.minDepthBounds = 0.f;
	m_DepthStencil.maxDepthBounds = 1.f;
}

bool Pipelines::LoadShaderModule(const char* filePath,
	VkDevice device,
	VkShaderModule* outShaderModule)
{

	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);

	file.read((char*)buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}