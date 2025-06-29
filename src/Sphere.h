#pragma once

#include <tuple>

#include <glm.hpp>

#include "Ray.h"

class Sphere
{
public:
	Sphere(const glm::vec3& position, const glm::vec3& color, float radius);

	bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const;

	float GetRadius() const { return m_Radius; }
	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetColor() const { return m_Color; }
private:
	glm::vec3 m_Position;
	glm::vec3 m_Color;

	float m_Radius;
};