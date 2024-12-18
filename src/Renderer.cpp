#include "Renderer.h"

Renderer::Renderer() {}

void Renderer::Init()
{
	m_Engine.Init();
}

void Renderer::SetWindow(GLFWwindow* window)
{
	m_Engine.SetWindow(window);
}


void Renderer::Render()
{
	m_Engine.DrawFrame();
}