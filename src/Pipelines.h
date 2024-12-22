#pragma once

#include <vector>
#include <fstream>

#include "Types.h"

class PipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;

	VkPipelineInputAssemblyStateCreateInfo m_InputAssembly;
	VkPipelineRasterizationStateCreateInfo m_Rasterizer;
	VkPipelineColorBlendAttachmentState m_ColorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo m_Multisampling;
	VkPipelineLayout m_PipelineLayout;
	VkPipelineDepthStencilStateCreateInfo m_DepthStencil;
	VkPipelineRenderingCreateInfo m_RenderInfo;
	VkFormat m_ColorAttachmentformat;

	PipelineBuilder() { Clear(); }

	void Clear();

	VkPipeline BuildPipeline(VkDevice device);
	void SetShaders(VkShaderModule vertexShader, const char* vertName, VkShaderModule fragmentShader, const char* fragName);
	void SetInputTopology(VkPrimitiveTopology topology);
	void SetPolygonMode(VkPolygonMode mode);
	void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
	void SetMultisamplingNone();
	void DisableBlending();
	void EnableBlendingAdditive();
	void EnableBlendingAlphablend();

	void SetColorAttachmentFormat(VkFormat format);
	void SetDepthFormat(VkFormat format);
	void DisableDepthtest();
	void EnableDepthtest(bool depthWriteEnable, VkCompareOp op);
};

namespace Pipelines {
	bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}
