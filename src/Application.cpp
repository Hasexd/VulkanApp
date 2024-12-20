#include "Application.h"

static void ErrorCallback(int error, const char* description)
{
	printf("Error: %s\n", description);
}


Application::Application(uint32_t width, uint32_t height, const char* title, bool resizable)
{
	Init(width, height, title, resizable);
}

Application::~Application()
{
	Cleanup();
}

void Application::Run()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();
		m_Engine.DrawFrame();
	}

}


void Application::Init(uint32_t width, uint32_t height, const char* title, bool resizable)
{
	if (!glfwInit())
	{
		printf("Couldn't initialize GLFW");
		return;
	}
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, resizable);
	m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	m_Engine.SetWindow(m_Window);
	m_Engine.Init();
}

void Application::Cleanup()
{
	m_Engine.Cleanup();
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}