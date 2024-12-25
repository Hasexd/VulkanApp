#include "Camera.h"


Camera::Camera():
	m_Position({0.f, 0.f, 1.f})
{
	float yawRad = glm::radians(m_Yaw);
	float pitchRad = glm::radians(m_Pitch);

	m_FrontVector.x = glm::cos(yawRad) * glm::cos(pitchRad);
	m_FrontVector.y = glm::sin(pitchRad);
	m_FrontVector.z = glm::sin(yawRad) * glm::cos(pitchRad);
	m_FrontVector = glm::normalize(m_FrontVector);

	m_RightVector = glm::normalize(glm::cross(m_FrontVector, s_WorldUp));
	m_UpVector = glm::normalize(glm::cross(m_RightVector, m_FrontVector));
}

void Camera::Rotate(double xOffset, double yOffset, double deltaTime)
{
	m_Yaw += xOffset * m_Sensitivity * (float)deltaTime;
	m_Pitch += yOffset * m_Sensitivity * (float)deltaTime;
	m_Pitch = glm::clamp(m_Pitch, -89.f, 89.f);

	float yawRad = glm::radians(m_Yaw);
	float pitchRad = glm::radians(m_Pitch);

	m_FrontVector.x = glm::cos(yawRad) * glm::cos(pitchRad);
	m_FrontVector.y = glm::sin(pitchRad);
	m_FrontVector.z = glm::sin(yawRad) * glm::cos(pitchRad);
	m_FrontVector = glm::normalize(m_FrontVector);

	m_RightVector = glm::normalize(glm::cross(m_FrontVector, s_WorldUp));
	m_UpVector = glm::normalize(glm::cross(m_RightVector, m_FrontVector));
}


void Camera::Move(const glm::vec3& movement, double deltaTime)
{
	m_Position += movement * m_Speed * (float)deltaTime;
}
