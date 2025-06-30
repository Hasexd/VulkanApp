#pragma once

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

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	void Resize(uint32_t width, uint32_t height);

	void Render();
	glm::vec4 RayGen(const glm::vec2& coord) const;


	HitPayload TraceRay(const Ray& ray) const;
	HitPayload ClosestHit(const Ray& ray, float hitDistance, uint32_t objectIndex) const;
	HitPayload Miss(const Ray& ray) const;

	uint32_t* GetData() const { return m_PixelData.get(); };

	std::vector<Sphere> GetSpheres() const { return m_Spheres; }
	std::vector<Sphere>& GetSpheres()  { return m_Spheres; }

	std::vector<Material> GetMaterials() const { return m_Materials; }

	Camera& GetCamera() { return m_Camera; }

	void ResetAccumulation();
	void SetAccumulation(bool enabled) { m_AccumulationEnabled = enabled; }
	bool IsAccumulationEnabled() const { return m_AccumulationEnabled; }

	void SetMaxRayBounces(uint32_t bounces) { m_MaxRayBounces = bounces; }
	uint32_t GetMaxRayBounces() const { return m_MaxRayBounces; }
	uint32_t& GetMaxRayBounces() { return m_MaxRayBounces; }


	void SetMaxSamples(uint32_t maxSamples) { m_MaxSamples = maxSamples; }
	uint32_t GetMaxSamples() const { return m_MaxSamples; }
	uint32_t& GetMaxSamples() { return m_MaxSamples; }


	bool IsComplete() const { return m_AccumulationEnabled && m_SampleCount >= m_MaxSamples; }
private:
	uint32_t m_Width, m_Height;
	std::unique_ptr<uint32_t[]> m_PixelData;
	std::unique_ptr<glm::vec4[]> m_AccumulationBuffer;

	std::atomic<uint32_t> m_SampleCount = 0;
	uint32_t m_MaxRayBounces;
	uint32_t m_MaxSamples;
	bool m_AccumulationEnabled = false;

	Camera m_Camera;
	std::vector<Sphere> m_Spheres;
	std::vector<Material> m_Materials;

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.f, -1.f, 1.f));
	float m_AspectRatio;

	uint32_t m_ThreadCount;
};
