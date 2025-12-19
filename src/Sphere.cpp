	#include "Sphere.h"


Sphere::Sphere(const std::string& name, const glm::vec3& position, float radius, uint32_t materialIndex):
	m_Name(name), m_Position(position), m_Radius(radius), m_MaterialIndex(materialIndex)
{
	
}

