#include "App.h"


App::App()
{
	if(!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW.";
		exit(-1);
	}

	m_Window = Window(1080, 720, "VulkanApp");

	glfwMakeContextCurrent(m_Window.Get());
	glfwSwapInterval(1);
}


void App::Run() 
{
	while(!glfwWindowShouldClose(m_Window.Get()))
	{
		glfwPollEvents();


		glfwSwapBuffers(m_Window.Get());
	}
}


App::~App()
{
	glfwTerminate();
}