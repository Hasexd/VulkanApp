#pragma once

#include <glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/euler_angles.hpp>
#include <string>

class Plane
{
public:
	Plane(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, float width, float height, uint32_t materialIndex);

	const std::string& GetName() const { return m_Name; }
	const glm::vec3& GetPosition() const { return m_Position; }

	glm::vec3 GetNormal() const;
	uint32_t GetMaterialIndex() const { return m_MaterialIndex; }

	float GetWidth() const { return m_Width; }
	float GetHeight() const { return m_Height; }

	float& GetWidth() { return m_Width; }
	float& GetHeight() { return m_Height; }

	const glm::vec3& GetRotation() const { return m_Rotation; }
	glm::vec3& GetRotation() { return m_Rotation; }

	glm::vec3 GetTangent() const;
	glm::vec3 GetBitangent() const;

	glm::vec3& GetPosition() { return m_Position; }
	uint32_t& GetMaterialIndex() { return m_MaterialIndex; }

private:
	std::string m_Name;
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	float m_Width;
	float m_Height;
	uint32_t m_MaterialIndex;
};