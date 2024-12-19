#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>

#include "Window.h"
#include "Renderer.h"

class App 
{
public:
	App();
	void Run();

private:
	Window m_Window;
	Renderer m_Renderer;
};