#pragma once

#include <glm.hpp>

class Ray
{
public:
	Ray(const glm::vec3& origin, glm::vec3& direction, float scalar);

	glm::vec3 Origin;
	glm::vec3 Direction;
	float Scalar;
};