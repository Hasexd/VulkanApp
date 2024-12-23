#pragma once
#include <glm.hpp>

#include "Ray.h"

class Sphere
{
public:
	Sphere(const glm::vec3& position, float radius);
	bool Intersects(const Ray& ray);

	glm::vec3 Position;
	float Radius;
};