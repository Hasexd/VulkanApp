#pragma once

#include "Sphere.h"
#include "Material.h"
#include "Camera.h"

#include <vector>

struct Scene
{
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
	Camera Camera;
};
