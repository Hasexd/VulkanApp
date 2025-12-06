#pragma once

#include <vec4.hpp>

struct Material
{
	glm::vec3 Color {1.f};
	float Roughness;
	float Metallic = 0.f;
	float EmissionPower = 0.f;

	[[nodiscard]] bool IsEmissive() const { return EmissionPower > 0.f; }
};
