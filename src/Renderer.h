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
	uint32_t PerPixel(const glm::vec2& coord) const;

	uint32_t* GetData() const;
private:
	uint32_t m_Width, m_Height;
	float m_AspectRatio;
	uint32_t* m_PixelData;
	Sphere sphere;
};
