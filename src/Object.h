#pragma once

#include <glm.hpp>

#include "Ray.h"

class Object
{
public:
	Object(const glm::vec3& position, const glm::vec3& color);
	virtual ~Object() = default;

	virtual bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const = 0;

	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetColor() const { return m_Color; }
protected:
	glm::vec3 m_Position;
	glm::vec3 m_Color;
};