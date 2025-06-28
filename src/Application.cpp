#include "Application.h"

#include "imgui_internal.h"

static void ErrorCallback(int error, const char* description)
{
	std::println("Error: {}\n", description);
}


Application::Application(uint32_t width, uint32_t height, const char* title, bool resizable):
	m_Window(nullptr, glfwDestroyWindow), m_Renderer(std::make_unique<Renderer>(width, height)), m_Engine(std::make_unique<VulkanEngine>())
{
	Init(width, height, title, resizable);
}

Application::~Application()
{
	Cleanup();
}

void Application::Run()
{
	double lastFrame = glfwGetTime();
	Camera* camera = m_Renderer->GetCamera();

	while (!glfwWindowShouldClose(m_Window.get()))
	{
		double currentFrame = glfwGetTime();
		m_DeltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glm::vec3 movement(0.0f);

		if (glfwGetKey(m_Window.get(), GLFW_KEY_W) == GLFW_PRESS)
			movement -= camera->GetDirection();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_S) == GLFW_PRESS)
			movement += camera->GetDirection();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_A) == GLFW_PRESS)
			movement -= camera->GetRight();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_D) == GLFW_PRESS)
			movement += camera->GetRight();

		if (glm::length(movement) > 0.0f)
			movement = glm::normalize(movement);

		m_Renderer->MoveCamera(movement, m_DeltaTime);

		glfwPollEvents();
		if (m_Engine->ResizeRequested)
		{
			m_Engine->OnWindowResize(m_Width, m_Height);
			m_Renderer->Resize(m_Width, m_Height);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Information");
		ImGui::Text("Rendering took: %.3fms", m_LastRenderTime);
		ImGui::End();

		ImGui::Render();

		Render();
		m_Engine->DrawFrame(m_Renderer->GetData());
	}
}


void Application::Render()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	m_Renderer->Render();
	auto endTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float, std::milli> duration = endTime - startTime;
	m_LastRenderTime = duration.count();
}


void Application::Init(uint32_t width, uint32_t height, const char* title, bool resizable)
{
	if (!glfwInit())
	{
		std::println("Couldn't initialize GLFW");
		return;
	}
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, resizable);

	
	GLFWwindow* tempWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	m_Window.reset(tempWindow);
	m_Width = width;
	m_Height = height;

	m_Engine->SetWindow(m_Window.get());
	m_Engine->Init();


	glfwSetWindowUserPointer(m_Window.get(), this);
	glfwSetFramebufferSizeCallback(m_Window.get(), [](GLFWwindow* window, int width, int height) -> void
		{
			if (const auto instance = static_cast<Application*>(glfwGetWindowUserPointer(window)))
			{
				instance->m_Engine->ResizeRequested = true;
				instance->m_Width = width;
				instance->m_Height = height;
			}
		});

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(m_Window.get(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwGetCursorPos(m_Window.get(), &m_LastMouseX, &m_LastMouseY);

	glfwSetCursorPosCallback(m_Window.get(), [](GLFWwindow* window, double xpos, double ypos) -> void
		{
			auto instance = static_cast<Application*>(glfwGetWindowUserPointer(window));

			if (instance->m_FirstMouse) {
				instance->m_LastMouseX = xpos;
				instance->m_LastMouseY = ypos;
				instance->m_FirstMouse = false;
			}

			double xOffset = instance->m_LastMouseX - xpos;
			double yOffset = instance->m_LastMouseY - ypos;
			instance->m_LastMouseX = xpos;
			instance->m_LastMouseY = ypos;

			instance->m_Renderer->RotateCamera(xOffset, yOffset);
		});


}

void Application::Cleanup()
{
	m_Engine->Cleanup();
	glfwTerminate();
}