#pragma once

#include <cstdint>
#include <vector>
#include <memory>


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
	void RotateCamera(double xPos, double yPos);

	Camera* GetCamera() { return &m_Camera; }
private:

	uint32_t m_Width, m_Height;
	uint32_t* m_PixelData;

	std::vector<std::shared_ptr<Sphere>> m_Spheres;

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.f, 1.f, -1.f));
	float m_AspectRatio;

	Camera m_Camera;

	glm::vec4 s_BackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

};
