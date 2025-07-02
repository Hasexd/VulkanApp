#pragma once

#include "Sphere.h"
#include "Material.h"
#include "Camera.h"

#include <vector>

class Scene
{
public:
	Scene() = default;

	std::vector<Sphere>& GetSpheres() { return m_Spheres; }
	std::vector<Material>& GetMaterials() { return m_Materials; }
	std::vector<Camera>& GetCameras() { return m_Cameras; }

	Camera& GetActiveCamera() { return m_Cameras[m_ActiveCameraIndex]; }
	const Camera& GetActiveCamera() const { return m_Cameras[m_ActiveCameraIndex]; }

	uint32_t GetActiveCameraIndex() const { return m_ActiveCameraIndex; }
	void SetActiveCameraIndex(uint32_t index) { m_ActiveCameraIndex = index; }


	void SwitchCamera(int direction)
	{
		if (m_Cameras.empty())
			return;

		int newIndex = static_cast<int>(m_ActiveCameraIndex) + direction;

		if (newIndex < 0)
			newIndex = static_cast<int>(m_Cameras.size()) - 1;
		else if (static_cast<size_t>(newIndex) >= m_Cameras.size())
			newIndex = 0;

		m_ActiveCameraIndex = static_cast<uint32_t>(newIndex);
	}

private:
	std::vector<Sphere> m_Spheres;
	std::vector<Material> m_Materials;
	std::vector<Camera> m_Cameras;

	uint32_t m_ActiveCameraIndex = 0;
};
