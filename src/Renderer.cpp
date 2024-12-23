#include "Renderer.h"

Renderer::Renderer(uint32_t width, uint32_t height):
	m_Width(width), m_Height(height), m_PixelData(new uint32_t[width * height]) {}


void Renderer::Resize(uint32_t width, uint32_t height)
{
	delete[] m_PixelData;
	m_PixelData = new uint32_t[width * height];

	m_Width = width;
	m_Height = height;
}


void Renderer::Render()
{
	for (uint32_t y = 0; y < m_Height; y++)
	{
		for (uint32_t x = 0; x < m_Width; x++)
		{
			//glm::vec2 coord = { (float)x / m_Width, float(y) / m_Height };
			//coord = coord * 2.f - 1.f;
			m_PixelData[x + y * m_Width] = Random::UInt();
			m_PixelData[x + y * m_Width] |= 0xff000000;
		}
	}
}


uint32_t Renderer::PerPixel(const glm::vec2& coord) const
{
	uint8_t r = (uint8_t)(coord.x * 255.f);
	uint8_t g = (uint8_t)(coord.y * 255.f);
	//Ray ray({coord.x, coord.y, 1}, {coord.x, coord.y, -1.f}, 4.f);


	return 0xff000000 | (g << 8) | r;
}



uint32_t* Renderer::GetData() const
{
	return m_PixelData;
}
