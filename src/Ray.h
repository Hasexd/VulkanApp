#pragma once

#include <glm.hpp>


struct HitPayload
{
	float HitDistance;
	glm::vec3 WorldPosition;
	glm::vec3 WorldNormal;
	uint32_t ObjectIndex;
};

class Ray
{
public:
	Ray(const glm::vec3& origin, const glm::vec3& direction);

	glm::vec3 Origin;
	glm::vec3 Direction;
};