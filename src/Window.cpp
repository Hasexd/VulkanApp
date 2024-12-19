#include "Window.h"


Window::Window():
	m_Width(0), m_Height(0), m_Window(nullptr, nullptr) { }

Window::Window(uint32_t width, uint32_t height, const char* title):
	m_Width(width), m_Height(height), m_Window(glfwCreateWindow(width, height, title, nullptr, nullptr), glfwDestroyWindow) 
{

	glfwSetWindowUserPointer(m_Window.get(), this);
	glfwSetWindowRefreshCallback(m_Window.get(), [](GLFWwindow* window) -> void 
		{
			int width, height;
			glfwGetWindowSize(window, &width, &height);

			Window* w = static_cast<Window*>(glfwGetWindowUserPointer(window));
			w->SetWidth((uint32_t)width);
			w->SetHeight((uint32_t)height);

		});
}

void Window::Cleanup()
{
	m_Window.reset();
}

void Window::SetWidth(uint32_t width)
{
	m_Width = width;
}

void Window::SetHeight(uint32_t height)
{
	m_Height = height;
}


GLFWwindow* Window::Get() const 
{
	return m_Window.get();
}


uint32_t Window::Width() const 
{
	return m_Width;
}


uint32_t Window::Height() const
{
	return m_Height;
}
