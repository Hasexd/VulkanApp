#pragma once

#include <tuple>

#include <glm.hpp>

#include "Ray.h"
#include "Material.h"

class Sphere
{
public:
	Sphere(const glm::vec3& position, float radius, const Material& material);

	bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const;

	float GetRadius() const { return m_Radius; }
	glm::vec3 GetPosition() const { return m_Position; }
	Material GetMaterial() const { return m_Material; }

	float& GetRadius()  { return m_Radius; }
	glm::vec3& GetPosition()  { return m_Position; }
	Material& GetMaterial()  { return m_Material; }

private:
	glm::vec3 m_Position;
	float m_Radius;

	Material m_Material;
};