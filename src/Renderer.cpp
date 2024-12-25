#include "Renderer.h"


static uint32_t ConvertToRGBA(const glm::vec4 color)
{
	uint8_t r = (uint8_t)(color.r * 255.f);
	uint8_t g = (uint8_t)(color.g * 255.f);
	uint8_t b = (uint8_t)(color.b * 255.f);
	uint8_t a = (uint8_t)(color.a * 255.f);


	return (a << 24) | (b << 16) | (g << 8) | r;
}


Renderer::Renderer(uint32_t width, uint32_t height):
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height), m_PixelData(new uint32_t[width * height]),
	sphere({ 0, 0, 0 }, 0.5f, glm::vec4(1.f, 1.f, 0.f, 1))
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

			glm::vec4 color = PerPixel(coord);
			color = glm::clamp(color, glm::vec4(0.f), glm::vec4(1.f));
			m_PixelData[x + y * m_Width] = ConvertToRGBA(color);
		}
	}
}


glm::vec4 Renderer::PerPixel(const glm::vec2& coord) const
{
	glm::vec3 rayOrigin(0.f, 0.f, 1.0f);
	glm::vec3 rayDirection(coord.x * m_AspectRatio * m_Scalar, coord.y * m_Scalar, -1.f);
	rayDirection = glm::normalize(rayDirection);

	Ray ray(rayOrigin, rayDirection);

	if (glm::vec3 hitNear, hitFar; sphere.Intersects(ray,hitNear, hitFar))
	{
		glm::vec3 nearNormal = glm::normalize(hitNear - sphere.Position);

		float angle = glm::max(glm::dot(nearNormal, -lightDir), 0.f);


		glm::vec3 sphereColor = nearNormal * angle;
		return glm::vec4(sphereColor, 1.f);
	}

	return glm::vec4(0.f, 0.f, 0.f ,1.f);

}


uint32_t* Renderer::GetData() const
{
	return m_PixelData;
}
