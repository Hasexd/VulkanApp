#pragma once
#include <cstdint>

#include <glm/glm.hpp>

#include "Random.h"
#include "Ray.h"

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
	uint32_t* m_PixelData;
};