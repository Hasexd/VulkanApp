#pragma once

#include <tuple>

#include <glm.hpp>

#include "Ray.h"
#include "Material.h"

class Sphere
{
public:
	Sphere(const glm::vec3& position, float radius, uint32_t materialIndex);

	bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const;

	float GetRadius() const { return m_Radius; }
	glm::vec3 GetPosition() const { return m_Position; }
	uint32_t GetMaterialIndex() const { return m_MaterialIndex; }

	float& GetRadius()  { return m_Radius; }
	glm::vec3& GetPosition()  { return m_Position; }
	uint32_t& GetMaterialIndex()  { return m_MaterialIndex; }

private:
	glm::vec3 m_Position;
	float m_Radius;
	uint32_t m_MaterialIndex;
};