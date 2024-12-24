	#include "Sphere.h"


Sphere::Sphere(const glm::vec3& position, float radius):
	Position(position), Radius(radius)
{
	
}


bool Sphere::Intersects(const Ray& ray) const
{
	float a = glm::dot(ray.Direction, ray.Direction);
	float b = 2.f * glm::dot(ray.Origin, ray.Direction);
	float c = glm::dot(ray.Origin, ray.Origin) - Radius * Radius;

	float discriminant = b * b - 4.f * a * c;

	if (discriminant >= 0)
		return true;


	return false;
}


