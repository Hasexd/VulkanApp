#pragma once
#include <cstdint>

#include <glm.hpp>

#include "Camera.h"
#include "Random.h"
#include "Ray.h"
#include "Sphere.h"

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	void Resize(uint32_t width, uint32_t height);

	void Render();
	glm::vec4 PerPixel(const glm::vec2& coord) const;
	uint32_t* GetData() const;

	void MoveCamera(const glm::vec3& movement, double deltaTime);
	void RotateCamera(double xPos, double yPos, double deltaTime);

	Camera* GetCamera() { return &m_Camera; }
private:
	uint32_t m_Width, m_Height;
	uint32_t* m_PixelData;

	Sphere sphere;

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.f, 1.f, -1.f));
	float m_AspectRatio;

	Camera m_Camera;
	double m_LastX = 0.f, m_LastY = 0.f;
	bool m_FirstMove = true;

};
