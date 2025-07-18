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
#include "Random.h"
#include "Ray.h"
#include "Sphere.h"
#include "Scene.h"
#include "VulkanEngine.h"


class Renderer
{
public:
	Renderer(const std::shared_ptr<GLFWwindow>& window, uint32_t width, uint32_t height);
	~Renderer();
	void Resize(uint32_t width, uint32_t height);

	void Render();


	void SetScene(const std::shared_ptr<Scene>& scene) { m_CurrentScene = scene; }

	void ResetAccumulation();
	void SetAccumulation(bool enabled) { m_AccumulationEnabled = enabled; }
	bool IsAccumulationEnabled() const { return m_AccumulationEnabled; }

	void SetMaxRayBounces(uint32_t bounces) { m_MaxRayBounces = bounces; }
	uint32_t GetMaxRayBounces() const { return m_MaxRayBounces; }
	uint32_t& GetMaxRayBounces() { return m_MaxRayBounces; }


	void SetMaxSamples(uint32_t maxSamples) { m_MaxSamples = maxSamples; }
	uint32_t GetMaxSamples() const { return m_MaxSamples; }
	uint32_t& GetMaxSamples() { return m_MaxSamples; }

	const RenderTime& GetRenderTime() const { return m_Engine->GetRenderTime(); }
	ImTextureID GetRenderTextureID() const { return m_Engine->GetRenderTextureID(); }

	bool IsComplete() const { return m_AccumulationEnabled && m_SampleCount >= m_MaxSamples; }
private:
	void UpdateUniformBuffer() const;
	void UpdateSphereBuffer() const;
	void UpdateMaterialBuffer() const;
private:
	std::unique_ptr<VulkanEngine> m_Engine;
	uint32_t m_Width, m_Height;
	float m_AspectRatio;

	std::weak_ptr<Scene> m_CurrentScene;

	uint32_t m_SampleCount = 0;
	uint32_t m_MaxRayBounces;
	uint32_t m_MaxSamples;
	bool m_AccumulationEnabled = false;
	bool m_DispatchCompute;
};
