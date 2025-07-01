#pragma once

#include <glm.hpp>

class Camera
{
public:
	Camera();
	void Move(const glm::vec3& movement, double deltaTime);
	void Rotate(double xOffset, double yOffset);

	glm::vec3 GetPosition() const { return m_Position; }
	glm::vec3 GetDirection() const { return m_FrontVector; }
	glm::vec3 GetRight() const { return m_RightVector; }
	glm::vec3 GetUp() const { return m_UpVector; }
	float GetFieldOfView() const { return m_FieldOfView; }
	float GetPitch() const { return m_Pitch; }
	float GetYaw() const { return m_Yaw; }

	void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateVectors(); }
	void SetFieldOfView(float fov) { m_FieldOfView = fov; }
	void SetRotation(float pitch, float yaw) { m_Yaw = yaw; m_Pitch = pitch; RecalculateVectors(); }

	
private:
	void RecalculateVectors();
private:
	glm::vec3 m_Position;
	glm::vec3 m_FrontVector;
	glm::vec3 m_UpVector;
	glm::vec3 m_RightVector;

	float m_Yaw = -90.f;
	float m_Pitch = 0.f;
	float m_Sensitivity = 0.1f;
	float m_FieldOfView = 90.f;
	float m_Speed = 3.f;

	static constexpr glm::vec3 s_WorldUp = { 0.f, 1.f, 0.f };

};