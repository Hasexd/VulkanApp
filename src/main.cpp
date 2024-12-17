#include <iostream>

#include "Window.h"

int main()
{
	glfwInit();
	Window window(1920, 1080, "VulkanApp");

	glfwMakeContextCurrent(window.Get());

	while (!glfwWindowShouldClose(window.Get())) {
		glfwPollEvents();
		glfwSwapBuffers(window.Get());
		glfwSwapInterval(1);
	}
	glfwTerminate();
}