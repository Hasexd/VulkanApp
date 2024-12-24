#include "Renderer.h"

Renderer::Renderer(uint32_t width, uint32_t height):
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height), m_PixelData(new uint32_t[width * height]),
	sphere({0, 0, 0.f}, 0.5f)
{}


void Renderer::Resize(uint32_t width, uint32_t height)
{
	delete[] m_PixelData;
	m_PixelData = new uint32_t[width * height];

	m_Width = width;
	m_Height = height;

	m_AspectRatio = (float)m_Width / m_Height;
}


void Renderer::Render()
{
	for (uint32_t y = 0; y < m_Height; y++)
	{
		for (uint32_t x = 0; x < m_Width; x++)
		{
			glm::vec2 coord = { (float)x / m_Width, float(y) / m_Height };
			coord = coord * 2.f - 1.f;
			m_PixelData[x + y * m_Width] = PerPixel(coord);
		}
	}
}


uint32_t Renderer::PerPixel(const glm::vec2& coord) const
{
	glm::vec3 rayOrigin(0.f, 0.f, 2.0f);
	glm::vec3 rayDirection(coord.x * m_AspectRatio, coord.y, -1.f);
	float scalar = glm::tan(glm::radians(90.f) / 2);

	Ray ray(rayOrigin, rayDirection, scalar);

	if (sphere.Intersects(ray))
	{
		return 0xff00ffff;
	}

	return 0x0000000;
}



uint32_t* Renderer::GetData() const
{
	return m_PixelData;
}
