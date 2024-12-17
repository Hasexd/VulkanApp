#pragma once


#include <memory>
#include <GLFW/glfw3.h>


class Window {
public:
	Window();
	Window(uint32_t width, uint32_t height, const char* title);
	GLFWwindow* Get() const;
private:
	uint32_t m_Width, m_Height;
	std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_Window;
};