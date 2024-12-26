#pragma once

#include <tuple>

#include <glm.hpp>

#include "Object.h"
#include "Ray.h"

class Sphere : public Object
{
public:
	Sphere(const glm::vec3& position, const glm::vec3& color, float radius);

	virtual bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const override;

	float GetRadius() const { return m_Radius; }
private:
	float m_Radius;
};