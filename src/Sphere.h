#pragma once

#include <tuple>

#include <glm.hpp>
#include "Ray.h"

class Sphere
{
public:
	Sphere(const glm::vec3& position, float radius, const glm::vec4& Color);
	bool Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const;

	glm::vec3 Position;
	float Radius;
	glm::vec4 Color;
};