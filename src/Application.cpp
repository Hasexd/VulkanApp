#include "Application.h"

#include "imgui_internal.h"

static void ErrorCallback(int error, const char* description)
{
	printf("Error: %s\n", description);
}


Application::Application(uint32_t width, uint32_t height, const char* title, bool resizable):
	m_Renderer(width, height)
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
			m_Renderer.Resize(m_Width, m_Height);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Information");
		ImGui::Text("Rendering took: %.3fms", m_LastRenderTime);
		ImGui::End();

		ImGui::Render();

		Render();
		m_Engine.DrawFrame(m_Renderer.GetData());
	}

}


void Application::Render()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	m_Renderer.Render();
	auto endTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float, std::milli> duration = endTime - startTime;
	m_LastRenderTime = duration.count();
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