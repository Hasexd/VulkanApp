#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <memory>


class Window 
{
public:
	Window();
	Window(uint32_t width, uint32_t height, const char* title);
	GLFWwindow* Get() const;

	uint32_t Width() const;
	uint32_t Height() const;

	void SetWidth(uint32_t width);
	void SetHeight(uint32_t height);

private:
	uint32_t m_Width, m_Height;
	std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
};