#include "Window.h"


Window::Window():
	m_Window(nullptr, nullptr) { }

Window::Window(uint32_t width, uint32_t height, const char* title):
	m_Width(width), m_Height(height), m_Window(glfwCreateWindow(width, height, title, nullptr, nullptr), glfwDestroyWindow) { }


GLFWwindow* Window::Get() const 
{
	return m_Window.get();
}