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
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("Background"))
		{
			ComputeEffect& selected = m_Engine.GetBackgroundEffects()[m_Engine.GetCurrentBackgroundEffects()];
			ImGui::Text("Selected effect: ", selected.Name);
			ImGui::SliderInt("Effect index:", &m_Engine.GetCurrentBackgroundEffects(), 0, m_Engine.GetBackgroundEffects().size() - 1);

			ImGui::InputFloat4("Data1", reinterpret_cast<float*>(&selected.Data.Data1));
			ImGui::InputFloat4("Data2", reinterpret_cast<float*>(&selected.Data.Data2));
			ImGui::InputFloat4("Data3", reinterpret_cast<float*>(&selected.Data.Data3));
			ImGui::InputFloat4("Data4", reinterpret_cast<float*>(&selected.Data.Data4));

		}
		ImGui::End();



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

	glfwSetWindowUserPointer(m_Window, this);
	glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) -> void
		{
			auto* instance = static_cast<Application*>(glfwGetWindowUserPointer(window));

			if (instance)
			{
				instance->m_Engine.OnWindowResize(width, height);
			}
		});

	m_Engine.SetWindow(m_Window);
	m_Engine.Init();
}

void Application::Cleanup()
{
	m_Engine.Cleanup();
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}