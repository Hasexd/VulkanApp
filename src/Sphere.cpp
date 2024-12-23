#include "Sphere.h"


Sphere::Sphere(const glm::vec3& position, float radius):
	Position(position), Radius(radius)
{
	
}


bool Sphere::Intersects(const Ray& ray)
{
	float determinant = std::pow((ray.Origin.x + ray.Direction.x * ray.Scalar - Position.x), 2)
	+ std::pow((ray.Origin.y + ray.Direction.y * ray.Scalar - Position.y), 2) + 
		std::pow(ray.Origin.z + ray.Direction.z * ray.Scalar - Position.z, 2) - Radius * Radius;

	return false;
}


