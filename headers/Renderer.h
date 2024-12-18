#pragma once


#include <GLFW/glfw3.h>

#include "VulkanEngine.h"


class Renderer
{
public:
	Renderer();
	void Render();

	void SetWindow(GLFWwindow* window);
	void Init();
private:
	VulkanEngine m_Engine;
};



