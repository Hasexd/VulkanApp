#pragma once

#include <glm.hpp>
#include <string>

class Plane
{
public:
	Plane(const std::string& name, const glm::vec3& position, const glm::vec3& normal, float width, float height, uint32_t materialIndex);

	const std::string& GetName() const { return m_Name; }
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetNormal() const { return m_Normal; }
	uint32_t GetMaterialIndex() const { return m_MaterialIndex; }

	float GetWidth() const { return m_Width; }
	float GetHeight() const { return m_Height; }

	float& GetWidth() { return m_Width; }
	float& GetHeight() { return m_Height; }

	glm::vec3& GetPosition() { return m_Position; }
	glm::vec3& GetNormal() { return m_Normal; }
	uint32_t& GetMaterialIndex() { return m_MaterialIndex; }
private:
	std::string m_Name;
	glm::vec3 m_Position;
	glm::vec3 m_Normal;
	float m_Width;
	float m_Height;
	uint32_t m_MaterialIndex;
};