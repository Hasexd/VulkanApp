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

	uint32_t PcgHash(uint32_t input)
	{
		uint32_t state = input * 747796405u + 2891336453u;
		uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;

		return (word >> 22u) ^ word;
	}

	float RandomFloat(uint32_t& seed)
	{
		seed = PcgHash(seed);
		return static_cast<float>(seed) / static_cast<float>(std::numeric_limits<uint32_t>::max());
	}

	glm::vec3 RandomCosineWeightedHemisphere(const glm::vec3& normal, uint32_t& seed)
	{

		float r1 = RandomFloat(seed);
		float r2 = RandomFloat(seed);

		float cosTheta = std::sqrt(r1);
		float sinTheta = std::sqrt(1.0f - r1);
		float phi = 2.0f * M_PI * r2;

		glm::vec3 w = normal;
		glm::vec3 u = glm::normalize(glm::cross((std::abs(w.x) > 0.1f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)), w));
		glm::vec3 v = glm::cross(w, u);

		return cosTheta * w + sinTheta * std::cos(phi) * u + sinTheta * std::sin(phi) * v;
	}
	constexpr glm::vec3 c_BackgroundColor = { 0.1f, 0.2f, 0.4f };
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
	std::shared_ptr<Scene> scene;

	if ((scene = m_CurrentScene.lock()) == nullptr)
		return;

	if (IsComplete())
		return;

	if (m_AccumulationEnabled)
		++m_SampleCount;

	AsyncTileBasedRendering(scene);
	
}


void Renderer::AsyncTileBasedRendering(const std::shared_ptr<Scene>& scene)
{
	constexpr uint32_t tileSize = 64;
	const uint32_t tilesX = (m_Width + tileSize - 1) / tileSize;
	const uint32_t tilesY = (m_Height + tileSize - 1) / tileSize;
	const uint32_t totalTiles = tilesX + tilesY;

	std::vector<std::future<void>> futures;
	futures.reserve(totalTiles);

	uint32_t currentSample = m_SampleCount;

	for (uint32_t tileY = 0; tileY < tilesY; tileY++)
	{
		for (uint32_t tileX = 0; tileX < tilesX; tileX++)
		{
			futures.emplace_back(std::async(std::launch::async, [this, scene, currentSample, tileX, tileY, tileSize]()
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
							coord.y = -coord.y;

							glm::vec4 color = RayGen(coord, currentSample, scene);
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


glm::vec4 Renderer::RayGen(const glm::vec2& coord, uint32_t sampleCount, const std::shared_ptr<Scene>& scene) const
{

	const Camera& camera = scene->GetActiveCamera();

	const float scalar = glm::tan(glm::radians(camera.GetFieldOfView()) / 2);

	glm::vec3 rayDirection(
		coord.x * m_AspectRatio * scalar,
		coord.y * scalar,
		-1.0f
	);

	rayDirection = glm::normalize(
		rayDirection.x * camera.GetRight() +
		rayDirection.y * camera.GetUp() +
		rayDirection.z * camera.GetDirection()
	);

	Ray ray(camera.GetPosition(), rayDirection);

	glm::vec3 light(0.f);
	glm::vec3 throughput(1.f);

	uint32_t seed = static_cast<uint32_t>(coord.x * 1000000.0f) +
		static_cast<uint32_t>(coord.y * 1000000.0f) * m_Width +
		sampleCount * 982451653u;

	for (size_t i = 0; i < m_MaxRayBounces; i++)
	{
		const HitPayload& hit = TraceRay(ray, scene);

		if (hit.HitDistance > 0.f)
		{
			const Material& material = scene->GetMaterials()[scene->GetSpheres()[hit.ObjectIndex].GetMaterialIndex()];

			light += material.EmissionColor * material.EmissionPower;
			throughput *= material.Color;


			ray.Origin = hit.WorldPosition + hit.WorldNormal * c_Epsilon;
			ray.Direction = RandomCosineWeightedHemisphere(hit.WorldNormal, seed);
		}
		else
		{
			//light += c_BackgroundColor * throughput;
			break;
		}
	}

	return {light, 1.f};
}

HitPayload Renderer::TraceRay(const Ray& ray, const std::shared_ptr<Scene>& scene)
{
	uint32_t closestSphereIndex = std::numeric_limits<uint32_t>::max();
	float closestDistance = std::numeric_limits<float>::max();

	glm::vec3 hitNear{}, hitFar{};

	for (size_t i = 0; i < scene->GetSpheres().size(); i++)
	{
		if (scene->GetSpheres()[i].Intersects(ray, hitNear, hitFar))
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
		return ClosestHit(ray, closestDistance, closestSphereIndex, scene);
	}

	return Miss(ray);
}

HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, uint32_t objectIndex, const std::shared_ptr<Scene>& scene)
{
	HitPayload hit{};

	hit.HitDistance = hitDistance;
	hit.ObjectIndex = objectIndex;
	hit.WorldPosition = ray.Origin + ray.Direction * hitDistance;
	hit.WorldNormal = glm::normalize(hit.WorldPosition - scene->GetSpheres()[objectIndex].GetPosition());

	return hit;
}

HitPayload Renderer::Miss(const Ray& ray)
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
