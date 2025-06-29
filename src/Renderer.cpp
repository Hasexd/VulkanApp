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
	glm::vec3 RandomVec3(float lowerBound, float upperBound)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution dis((lowerBound), (upperBound));

		return glm::vec3(dis(gen), dis(gen), dis(gen));
	}

	constexpr glm::vec4 c_BackgroundColor = { 0.1f, 0.2f, 0.4f, 0.0f };
	constexpr int c_MaxRayBounces = 2;
	constexpr float c_Epsilon = 0.0001f;
}


Renderer::Renderer(uint32_t width, uint32_t height) :
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height), m_PixelData(std::make_unique<uint32_t[]>(m_Width * m_Height))
{

	Material pink = { {1.f, 0.f, 1.f, 1.f}, 0.1f, 0 };
	Material blue = { {0.f, 0.f, 1.f, 1.f}, 0.1f, 0 };

	m_Spheres.reserve(2);
	m_Spheres.emplace_back(Sphere({0.f, 0.f, 5.f}, 2.f, pink));
	m_Spheres.emplace_back(Sphere({ 0.f, 102.f, 0.f }, 100.f, blue));

	m_Width = width;
	m_Height = height;
	m_AspectRatio = (float)m_Width / m_Height;
}


void Renderer::Resize(uint32_t width, uint32_t height)
{
	m_PixelData.reset(new uint32_t[width * height]);

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

			glm::vec4 color = RayGen(coord);
			color = glm::clamp(color, glm::vec4(0.f), glm::vec4(1.f));
			m_PixelData[x + y * m_Width] = ConvertToRGBA(color);
		}
	}
}


glm::vec4 Renderer::RayGen(const glm::vec2& coord) const
{
	const float scalar = glm::tan(glm::radians(m_Camera.GetFieldOfView()) / 2);

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
	glm::vec4 color = c_BackgroundColor;
	float multiplier = 1.f;

	for (size_t i = 0; i < c_MaxRayBounces; i++)
	{
		const HitPayload& hit = TraceRay(ray);

		if (hit.HitDistance > 0.f)
		{
			const float angle = glm::max(glm::dot(hit.WorldNormal, -lightDir), 0.f);
			const Material& material = m_Spheres[hit.ObjectIndex].GetMaterial();

			const glm::vec4 sphereColor = material.Color * angle;
			color += sphereColor * multiplier;
			multiplier *= 0.5f;

			ray.Origin = hit.WorldPosition + hit.WorldNormal * c_Epsilon;
			ray.Direction = glm::reflect(ray.Direction, hit.WorldNormal + material.Roughness * RandomVec3(-0.5f, 0.5f));
		}
		else
		{
			color += c_BackgroundColor * multiplier;
		}
	}

	return color;
}

HitPayload Renderer::TraceRay(const Ray& ray) const
{
	uint32_t closestSphereIndex = std::numeric_limits<uint32_t>::max();
	float closestDistance = std::numeric_limits<float>::max();

	glm::vec3 hitNear{}, hitFar{};

	for (size_t i = 0; i < m_Spheres.size(); i++)
	{
		if (m_Spheres[i].Intersects(ray, hitNear, hitFar))
		{
			float distanceToNear = glm::dot(hitNear - ray.Origin, ray.Direction);
			float distanceToFar = glm::dot(hitFar - ray.Origin, ray.Direction);

			if (distanceToNear > 0.f && closestDistance > distanceToNear)
			{
				closestSphereIndex = i;
				closestDistance = distanceToNear;
			}
			else if (distanceToFar > 0.f && closestDistance > distanceToFar)
			{
				closestSphereIndex = i;
				closestDistance = distanceToFar;
			}
		}
	}

	if (closestSphereIndex < std::numeric_limits<uint32_t>::max())
	{
		return ClosestHit(ray, closestDistance, closestSphereIndex);
	}

	return Miss(ray);
}

HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, uint32_t objectIndex) const
{
	HitPayload hit{};

	hit.HitDistance = hitDistance;
	hit.ObjectIndex = objectIndex;
	hit.WorldPosition = ray.Origin + ray.Direction * hitDistance;
	hit.WorldNormal = glm::normalize(hit.WorldPosition - m_Spheres[objectIndex].GetPosition());

	return hit;
}

HitPayload Renderer::Miss(const Ray& ray) const
{
	HitPayload hit{};
	hit.HitDistance = -1.f;

	return hit;
}
