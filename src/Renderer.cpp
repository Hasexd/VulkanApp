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
		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd());
		std::uniform_real_distribution dis(lowerBound, upperBound);

		return {dis(gen), dis(gen), dis(gen)};
	}

	constexpr glm::vec4 c_BackgroundColor = { 0.1f, 0.2f, 0.4f, 0.0f };
	constexpr float c_Epsilon = 0.0001f;
}


Renderer::Renderer(uint32_t width, uint32_t height) :
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height),
	m_PixelData(std::make_unique<uint32_t[]>(m_Width * m_Height)),
	m_AccumulationBuffer(std::make_unique<glm::vec4[]>(m_Width * m_Height))
{
	

	m_Width = width;
	m_Height = height;
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;
	m_ThreadCount = std::thread::hardware_concurrency();

	m_MaxSamples = 250;
	m_MaxRayBounces = 10;

	if (m_ThreadCount == 0)
		m_ThreadCount = 4;
}


void Renderer::Resize(uint32_t width, uint32_t height)
{

	m_Width = width;
	m_Height = height;
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;

	m_PixelData.reset(new uint32_t[width * height]);
	m_AccumulationBuffer.reset(new glm::vec4[width * height]);
	m_SampleCount = 0;

}


void Renderer::Render()
{
	if (m_CurrentScene == nullptr)
		return;

	if (IsComplete())
		return;

	if (m_AccumulationEnabled)
		++m_SampleCount;

	const uint32_t tileSize = 64;
	const uint32_t tilesX = (m_Width + tileSize - 1) / tileSize;
	const uint32_t tilesY = (m_Height + tileSize - 1) / tileSize;
	const uint32_t totalTiles = tilesX + tilesY;

	std::vector<std::future<void>> futures;
	futures.reserve(totalTiles);

	for (uint32_t tileY = 0; tileY < tilesY; tileY++)
	{
		for (uint32_t tileX = 0; tileX < tilesX; tileX++)
		{
			futures.emplace_back(std::async(std::launch::async, [this, tileX, tileY, tileSize]()
				{
					const uint32_t startX = tileX * tileSize;
					const uint32_t startY = tileY * tileSize;
					const uint32_t endX = std::min(startX + tileSize, m_Width);
					const uint32_t endY = std::min(startY + tileSize, m_Height);

					for (uint32_t y = startY; y < endY; y++)
					{
						for (uint32_t x = startX; x < endX; x++)
						{
							glm::vec2 coord = { static_cast<float>(x) / m_Width, static_cast<float>(y) / m_Height };
							coord = coord * 2.f - 1.f;

							glm::vec4 color = RayGen(coord);
							color = glm::clamp(color, glm::vec4(0.f), glm::vec4(1.f));

							size_t idx = x + y * m_Width;

							if (m_AccumulationEnabled)
							{
								m_AccumulationBuffer[idx] += color;
								glm::vec4 avg = m_AccumulationBuffer[idx] / static_cast<float>(m_SampleCount);

								m_PixelData[idx] = ConvertToRGBA(avg);
							}
							else
							{
								m_PixelData[idx] = ConvertToRGBA(color);
							}
						}
					}
				}));
		}
	}

	for (auto& future : futures)
	{
		future.wait();
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

	for (size_t i = 0; i < m_MaxRayBounces; i++)
	{
		const HitPayload& hit = TraceRay(ray);

		if (hit.HitDistance > 0.f)
		{
			const float angle = glm::max(glm::dot(hit.WorldNormal, -lightDir), 0.f);
			const Material& material = m_CurrentScene->Materials[m_CurrentScene->Spheres[hit.ObjectIndex].GetMaterialIndex()];

			const glm::vec4 sphereColor = material.Color * angle;
			color += sphereColor * multiplier;
			multiplier *= 0.5f;

			ray.Origin = hit.WorldPosition + hit.WorldNormal * c_Epsilon;
			ray.Direction = glm::reflect(ray.Direction, hit.WorldNormal + material.Roughness * RandomVec3(-0.5f, 0.5f));
		}
		else
		{
			color += c_BackgroundColor * multiplier;
			break;
		}
	}

	return color;
}

HitPayload Renderer::TraceRay(const Ray& ray) const
{
	uint32_t closestSphereIndex = std::numeric_limits<uint32_t>::max();
	float closestDistance = std::numeric_limits<float>::max();

	glm::vec3 hitNear{}, hitFar{};

	for (size_t i = 0; i < m_CurrentScene->Spheres.size(); i++)
	{
		if (m_CurrentScene->Spheres[i].Intersects(ray, hitNear, hitFar))
		{
			const float distanceToNear = glm::dot(hitNear - ray.Origin, ray.Direction);
			const float distanceToFar = glm::dot(hitFar - ray.Origin, ray.Direction);

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
	hit.WorldNormal = glm::normalize(hit.WorldPosition - m_CurrentScene->Spheres[objectIndex].GetPosition());

	return hit;
}

HitPayload Renderer::Miss(const Ray& ray) const
{
	HitPayload hit{};
	hit.HitDistance = -1.f;

	return hit;
}


void Renderer::ResetAccumulation()
{
	std::fill_n(m_AccumulationBuffer.get(), m_Width * m_Height, glm::vec4(0.0f));
	m_SampleCount = 0;
}
