#include "Renderer.h"


void Renderer::Init()
{
	m_Engine.Init();
}

void Renderer::SetWindow(GLFWwindow* window)
{
	m_Window = window;
	m_Engine.SetWindow(window);
}


void Renderer::Render()
{
	m_Engine.DrawFrame();
}

void Renderer::DeviceWaitIdle()
{
	m_Engine.GetDispatchTable().deviceWaitIdle();
}