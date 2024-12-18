#include "App.h"


App::App()
{
	if(!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW.";
		exit(-1);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_Window = Window(1080, 720, "VulkanApp");
	m_Renderer.SetWindow(m_Window.Get());
	m_Renderer.Init();

	glfwMakeContextCurrent(m_Window.Get());
	glfwSwapInterval(1);
}


void App::Run() 
{
	while(!glfwWindowShouldClose(m_Window.Get()))
	{
		glfwPollEvents();
		m_Renderer.DrawFrame();
		glfwSwapBuffers(m_Window.Get());
	}
	m_Renderer.GetDispatchTable().deviceWaitIdle();
}


App::~App()
{
	glfwTerminate();
}