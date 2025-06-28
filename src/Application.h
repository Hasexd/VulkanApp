#pragma once


#include <print>
#include <memory>

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
private:
	void Render();
	void Init(uint32_t width, uint32_t height, const char* title, bool resizable);
	void Cleanup();
private:
	uint32_t m_Width, m_Height;

	std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<VulkanEngine> m_Engine;

	float m_LastRenderTime;
	double m_DeltaTime;
};