#pragma once

#include <vec4.hpp>

struct Material
{
	glm::vec4 Color {1.f};
	float Roughness;
	float Metallic = 0.f;
};
