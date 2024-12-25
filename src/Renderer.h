#pragma once
#include <cstdint>

#include <glm.hpp>

#include "Random.h"
#include "Ray.h"
#include "Sphere.h"

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	void Resize(uint32_t width, uint32_t height);

	void Render();
	glm::vec4 PerPixel(const glm::vec2& coord) const;

	uint32_t* GetData() const;
private:
	uint32_t m_Width, m_Height;
	uint32_t* m_PixelData;

	Sphere sphere;

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.f, 1.f, -1.f));

	float m_AspectRatio;
	float m_Scalar = glm::tan(glm::radians(100.f) / 2);

};
