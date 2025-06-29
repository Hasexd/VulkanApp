	#include "Sphere.h"


Sphere::Sphere(const glm::vec3& position, const glm::vec3& color, float radius):
	m_Position(position), m_Color(color), m_Radius(radius)
{
	
}

bool Sphere::Intersects(const Ray& ray, glm::vec3& outHitNear, glm::vec3& outHitFar) const
{
	glm::vec3 oc = ray.Origin - m_Position;

	float a = glm::dot(ray.Direction, ray.Direction);
	float b = 2.f * glm::dot(oc, ray.Direction);
	float c = glm::dot(oc, oc) - m_Radius * m_Radius;

	float discriminant = b * b - 4.f * a * c;

	if (discriminant >= 0.f)
	{
		float t1 = (-b - sqrt(discriminant)) / (2 * a);
		float t2 = (-b + sqrt(discriminant)) / (2 * a);

		outHitNear = ray.Origin + ray.Direction * t1;
		outHitFar = ray.Origin + ray.Direction * t2;

		return true;
	}

	return false;
}


