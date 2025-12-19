#include "Plane.h"

Plane::Plane(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, float width, float height, uint32_t materialIndex):
	m_Name(name), m_Position(position), m_Rotation(rotation), m_Width(width), m_Height(height), m_MaterialIndex(materialIndex)
{
	
}

glm::vec3 Plane::GetNormal() const
{
    glm::mat4 rotationMatrix = glm::eulerAngleXYZ(m_Rotation.x, m_Rotation.y, m_Rotation.z);
    return glm::vec3(rotationMatrix * glm::vec4(0, 1, 0, 0));
}

glm::vec3 Plane::GetTangent() const
{
	glm::mat4 rotationMatrix = glm::eulerAngleXYZ(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	return glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0));
}

glm::vec3 Plane::GetBitangent() const 
{
	glm::mat4 rotationMatrix = glm::eulerAngleXYZ(m_Rotation.x, m_Rotation.y, m_Rotation.z);
	return glm::vec3(rotationMatrix * glm::vec4(0, 0, 1, 0));
}