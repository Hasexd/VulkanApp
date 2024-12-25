#pragma once

#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanEngine.h"
#include "Renderer.h"

class Application
{
public:
	Application(uint32_t width, uint32_t height, const char* title, bool resizable);
	~Application();

	void Run();
	void Cleanup();
private:
	void Render();
	void Init(uint32_t width, uint32_t height, const char* title, bool resizable);
private:
	uint32_t m_Width, m_Height;
	GLFWwindow* m_Window;

	Renderer m_Renderer;
	VulkanEngine m_Engine;

	float m_LastRenderTime;
	double m_DeltaTime;
};