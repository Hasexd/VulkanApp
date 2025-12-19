#pragma once


#define _USE_MATH_DEFINES
#include <cmath>

#include <cstdint>
#include <vector>
#include <glm.hpp>
#include <random>
#include <future>
#include <thread>

#include "Camera.h"
#include "Ray.h"
#include "Sphere.h"
#include "Scene.h"
#include "VulkanEngine.h"


class Renderer
{
public:
	Renderer(const std::shared_ptr<GLFWwindow>& window, uint32_t width, uint32_t height);
	~Renderer();

	void OnWindowResize(uint32_t width, uint32_t height) const;
	void ResizeViewport(uint32_t width, uint32_t height);

	void Render();
	void ReloadShaders();

	void SetScene(const std::shared_ptr<Scene>& scene) { m_CurrentScene = scene; }

	void ResetAccumulation();
	void SetAccumulation(bool enabled) { m_AccumulationEnabled = enabled; }
	bool IsAccumulationEnabled() const { return m_AccumulationEnabled; }

	void SetMaxRayBounces(uint32_t bounces) { m_MaxRayBounces = bounces; }
	uint32_t GetMaxRayBounces() const { return m_MaxRayBounces; }
	uint32_t& GetMaxRayBounces() { return m_MaxRayBounces; }

	void SetBgColor(const glm::vec3& bgColor) { m_BackgroundColor = bgColor; }
	glm::vec3& GetBgColorRef() { return m_BackgroundColor; }


	void SetMaxSamples(uint32_t maxSamples) { m_MaxSamples = maxSamples; }
	uint32_t GetMaxSamples() const { return m_MaxSamples; }
	uint32_t& GetMaxSamples() { return m_MaxSamples; }

	float GetRenderTime() const { return m_Engine->GetRenderTime(); }
	ImTextureID GetRenderTextureID() const { return m_Engine->GetRenderTextureID(); }

	void SetBloomEnabled(bool enabled) { m_Engine->SetBloomEnabled(enabled); }
	void SetColorGradingEnabled(bool enabled) { m_Engine->SetColorGradingEnabled(enabled); }

	bool IsComplete() const { return m_AccumulationEnabled && m_SampleCount >= m_MaxSamples; }

	void SwitchLuts(LUTType type) { m_Engine->SwitchLuts(type); }
private:
	void UpdateUniformBuffer(const std::shared_ptr<Scene>& scene) const;
	void UpdateSphereBuffer(const std::shared_ptr<Scene>& scene) const;
	void UpdateMaterialBuffer(const std::shared_ptr<Scene>& scene) const;
	void UpdatePlaneBuffer(const std::shared_ptr<Scene>& scene) const;
private:
	std::unique_ptr<VulkanEngine> m_Engine;
	uint32_t m_Width, m_Height;
	float m_AspectRatio;

	std::weak_ptr<Scene> m_CurrentScene;

	glm::vec3 m_BackgroundColor = { 0.5f, 0.7f, 1.0f };
	uint32_t m_SampleCount = 0;
	uint32_t m_MaxRayBounces;
	uint32_t m_MaxSamples;
	bool m_AccumulationEnabled = false;
	bool m_DispatchCompute;
};
