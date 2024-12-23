#include "Application.h"

#include "imgui_internal.h"

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
		if (m_Engine.ResizeRequested)
		{
			m_Engine.OnWindowResize(m_Width, m_Height);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::EndFrame();
		ImGui::Render();
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
	m_Width = width;
	m_Height = height;

	m_Engine.SetWindow(m_Window);
	m_Engine.Init();


	glfwSetWindowUserPointer(m_Window, this);
	glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) -> void
		{
			auto* instance = static_cast<Application*>(glfwGetWindowUserPointer(window));
			if (instance)
			{
				instance->m_Engine.ResizeRequested = true;
				instance->m_Width = width;
				instance->m_Height = height;
			}
		});
}

void Application::Cleanup()
{
	m_Engine.Cleanup();
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}