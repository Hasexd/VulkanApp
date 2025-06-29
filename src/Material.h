#pragma once

#include <vec4.hpp>

struct Material
{
	glm::vec4 Color;
	float Roughness;
	float Reflectivity;
	bool Metallic;
};
