#pragma once

#include <memory>

#include <GLFW/glfw3.h>

#include "VulkanEngine.h"


class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;
	void Render();

	void SetWindow(GLFWwindow* window);
	void Init();
	void DeviceWaitIdle();
private:
	GLFWwindow* m_Window;
	VulkanEngine m_Engine;
};



