#include "Camera.h"

Camera::Camera() : m_Position(0.f, 0.f, 1.f) {
    RecalculateVectors();
}

void Camera::Rotate(double xOffset, double yOffset) {
    m_Yaw += xOffset * m_Sensitivity;
    m_Pitch -= yOffset * m_Sensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.f, 89.f);
    RecalculateVectors();
}

void Camera::Move(const glm::vec3& movement, double deltaTime) {
    m_Position += movement * m_Speed * static_cast<float>(deltaTime);
}

void Camera::RecalculateVectors() {
    float yawRad = glm::radians(m_Yaw);
    float pitchRad = glm::radians(m_Pitch);

    glm::vec3 newFront;
    newFront.x = cos(yawRad) * cos(pitchRad);
    newFront.y = sin(pitchRad);
    newFront.z = sin(yawRad) * cos(pitchRad);
    m_FrontVector = glm::normalize(newFront);

    m_RightVector = glm::normalize(glm::cross(m_FrontVector, s_WorldUp));
    m_UpVector = glm::normalize(glm::cross(m_RightVector, m_FrontVector));
}