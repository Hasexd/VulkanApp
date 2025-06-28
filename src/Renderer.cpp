#include "Renderer.h"

namespace 
{
	uint32_t ConvertToRGBA(const glm::vec4 color)
	{
		uint8_t r = (uint8_t)(color.r * 255.f);
		uint8_t g = (uint8_t)(color.g * 255.f);
		uint8_t b = (uint8_t)(color.b * 255.f);
		uint8_t a = (uint8_t)(color.a * 255.f);


		return (a << 24) | (b << 16) | (g << 8) | r;
	}
}


Renderer::Renderer(uint32_t width, uint32_t height) :
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height), m_PixelData(new uint32_t[m_Width * m_Height])
{
	m_Objects.reserve(2);
	m_Objects.emplace_back(new Sphere({0.f, 0.f, -1.f}, {1.f, 0.f, 1.f}, 3.f));
	m_Objects.emplace_back(new Sphere({ 3.f, 4.f, -1.f }, { 0.f, 0.f, 1.f }, 2.f));

	m_Width = width;
	m_Height = height;
	m_AspectRatio = (float)m_Width / m_Height;
	m_PixelData = new uint32_t[m_Width * m_Height];
}


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
    float scalar = glm::tan(glm::radians(m_Camera.GetFieldOfView()) / 2);

    glm::vec3 rayDirection(
        coord.x * m_AspectRatio * scalar,
        coord.y * scalar,
        -1.0f
    );

    rayDirection = glm::normalize(
        rayDirection.x * m_Camera.GetRight() +
        rayDirection.y * m_Camera.GetUp() +
        rayDirection.z * m_Camera.GetDirection()
    );

    Ray ray(m_Camera.GetPosition(), rayDirection);


	for (const auto& object : m_Objects)
	{
		if (glm::vec3 hitNear, hitFar; object->Intersects(ray, hitNear, hitFar))
		{
			float distanceToNear = glm::dot(hitNear - ray.Origin, ray.Direction);
			float distanceToFar = glm::dot(hitFar - ray.Origin, ray.Direction);

			glm::vec3 hitPoint;

			if (distanceToNear > 0.0f)
			{
				hitPoint = hitNear;
			}
			else if (distanceToFar > 0.0f)
			{
				hitPoint = hitFar;
			}
			else
			{
				return { 0.0f, 0.0f, 0.0f, 1.0f };
			}

			glm::vec3 normal = glm::normalize(hitPoint - object->GetPosition());
			float angle = glm::max(glm::dot(normal, -lightDir), 0.f);
			glm::vec3 sphereColor = object->GetColor() * angle;

			return { sphereColor, 1.f };
		}
	}
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

void Renderer::MoveCamera(const glm::vec3& movement, double deltaTime)
{
	m_Camera.Move(movement, deltaTime);
}

void Renderer::RotateCamera(double xPos, double yPos, double deltaTime)
{
	if (m_FirstMove)
	{
		m_LastX = xPos;
		m_LastY = yPos;
		m_FirstMove = false;
		return;
	}

	double xOffset = xPos - m_LastX;
	double yOffset = yPos - m_LastY;

	m_LastX = xPos;
	m_LastY = yPos;

	m_Camera.Rotate(xOffset, yOffset, deltaTime);
}

uint32_t* Renderer::GetData() const
{
	return m_PixelData;
}
