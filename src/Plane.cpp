#include "Plane.h"

Plane::Plane(const std::string& name, const glm::vec3& position, const glm::vec3& normal, float width, float height, uint32_t materialIndex):
	m_Name(name), m_Position(position), m_Normal(normal), m_Width(width), m_Height(height), m_MaterialIndex(materialIndex)
{
	
}