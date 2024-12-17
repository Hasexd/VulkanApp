#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "Window.h"

class App {
public:
	App();
	~App();
	void Run();

private:
	Window m_Window;
};