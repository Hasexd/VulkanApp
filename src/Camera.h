#pragma once

#include <glm.hpp>

class Camera
{
public:
	Camera();
	void Move(const glm::vec3& movement, double deltaTime);
	void Rotate(double xOffset, double yOffset, double deltaTime);

	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetDirection() const { return m_FrontVector; }
	glm::vec3 GetRight() const { return m_RightVector; }
	glm::vec3 GetUp() const { return m_UpVector; }
	float GetFieldOfView() const { return m_FieldOfView; }
private:
	glm::vec3 m_Position;
	glm::vec3 m_FrontVector;
	glm::vec3 m_UpVector;
	glm::vec3 m_RightVector;

	float m_Yaw = 0.f, m_Pitch = 0.f;
	float m_Sensitivity = 1.5f;
	float m_FieldOfView = 90.f;
	float m_Speed = 1.f;

	static constexpr glm::vec3 s_WorldUp = { 0.f, -1.f, 0.f };

};