#pragma once



#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanEngine.h"

class Application
{
public:
	Application(uint32_t width, uint32_t height, const char* title, bool resizable);
	~Application();

	void Run();
	void Cleanup();
private:
	void Init(uint32_t width, uint32_t height, const char* title, bool resizable);
private:
	uint32_t* m_PixelData;
	uint32_t m_Width, m_Height;
	GLFWwindow* m_Window;
	VulkanEngine m_Engine;
};