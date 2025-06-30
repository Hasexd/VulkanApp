#include "Application.h"

#include "imgui_internal.h"

static void ErrorCallback(int error, const char* description)
{
	std::println("Error: {}\n", description);
}

Application::Application(uint32_t width, uint32_t height, const char* title, bool resizable) :
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
	Camera& camera = m_Renderer->GetCamera();
	glm::vec3 lastCameraPos = camera.GetPosition();

	while (!glfwWindowShouldClose(m_Window.get()))
	{
		double currentFrame = glfwGetTime();
		m_DeltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();

		if (m_Engine->ResizeRequested)
		{
			m_Engine->OnWindowResize(m_Width, m_Height);
			m_Renderer->Resize(m_Width, m_Height);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		HandleCursorInput();
		HandleMouseInput(camera);

		bool cameraChanged = false;
		glm::vec3 currentCameraPos = camera.GetPosition();
		if (glm::distance(currentCameraPos, lastCameraPos) > 0.001f)
		{
			cameraChanged = true;
			lastCameraPos = currentCameraPos;
		}

		ImGui::Begin("Scene");

		bool sceneChanged = false;
		for (size_t i = 0; i < m_Renderer->GetSpheres().size(); i++)
		{
			Sphere& sphere = m_Renderer->GetSpheres()[i];

			ImGui::PushID(i);

			if (ImGui::DragFloat3("Position", glm::value_ptr(sphere.GetPosition()), 0.1f))
				sceneChanged = true;
			if (ImGui::DragFloat("Radius", &sphere.GetRadius(), 0.1f))
				sceneChanged = true;
			if (ImGui::ColorEdit3("Color", glm::value_ptr(sphere.GetMaterial().Color)))
				sceneChanged = true;
			if (ImGui::DragFloat("Roughness", &sphere.GetMaterial().Roughness, 0.01f, 0.0f, 1.0f))
				sceneChanged = true;

			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		ImGui::Begin("Information");
		ImGui::Text("Rendering took: %.3fms", m_LastRenderTime);

		ImGui::Separator();
		ImGui::Text("Accumulation Settings");

		bool accumulationEnabled = m_Renderer->IsAccumulationEnabled();
		if (ImGui::Checkbox("Enable Accumulation", &accumulationEnabled))
		{
			m_Renderer->SetAccumulation(accumulationEnabled);
			if (!accumulationEnabled)
			{
				m_Renderer->ResetAccumulation();
			}
		}

		if (accumulationEnabled)
		{
			if (ImGui::Button("Reset Accumulation"))
			{
				m_Renderer->ResetAccumulation();
			}

			if (m_Renderer->IsComplete())
			{
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Rendering Complete!");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Real-time Mode");
		}

		ImGui::End();

		ImGui::Render();

		glm::vec3 movement(0.0f);
		if (glfwGetKey(m_Window.get(), GLFW_KEY_W) == GLFW_PRESS)
			movement -= camera.GetDirection();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_S) == GLFW_PRESS)
			movement += camera.GetDirection();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_A) == GLFW_PRESS)
			movement -= camera.GetRight();
		if (glfwGetKey(m_Window.get(), GLFW_KEY_D) == GLFW_PRESS)
			movement += camera.GetRight();

		if (glm::length(movement) > 0.0f)
		{
			movement = glm::normalize(movement);
			cameraChanged = true;
		}

		camera.Move(movement, m_DeltaTime);

		if ((cameraChanged || sceneChanged) && m_Renderer->IsAccumulationEnabled())
		{
			m_Renderer->ResetAccumulation();
		}

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

void Application::HandleMouseInput(Camera& camera)
{
	if (m_CursorVisible || ImGui::GetIO().WantCaptureMouse)
		return;

	double xpos, ypos;
	glfwGetCursorPos(m_Window.get(), &xpos, &ypos);

	if (m_FirstMouse) {
		m_LastMouseX = xpos;
		m_LastMouseY = ypos;
		m_FirstMouse = false;
		return;
	}

	double xOffset = m_LastMouseX - xpos;
	double yOffset = m_LastMouseY - ypos;
	m_LastMouseX = xpos;
	m_LastMouseY = ypos;

	camera.Rotate(xOffset, yOffset);
}

void Application::HandleCursorInput()
{
	if (glfwGetKey(m_Window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS && !m_EscapePressed)
	{
		m_CursorVisible = true;
		glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		m_EscapePressed = true;
	}
	else if (glfwGetKey(m_Window.get(), GLFW_KEY_ESCAPE) == GLFW_RELEASE)
	{
		m_EscapePressed = false;
	}

	if (glfwGetMouseButton(m_Window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !m_LeftClickPressed && !ImGui::GetIO().WantCaptureMouse)
	{
		m_CursorVisible = false;
		glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		m_LeftClickPressed = true;
	}
	else if (glfwGetMouseButton(m_Window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
	{
		m_LeftClickPressed = false;
	}
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
	glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetFramebufferSizeCallback(m_Window.get(), [](GLFWwindow* window, int width, int height) -> void
		{
			if (const auto instance = static_cast<Application*>(glfwGetWindowUserPointer(window)))
			{
				instance->m_Engine->ResizeRequested = true;
				instance->m_Width = width;
				instance->m_Height = height;
			}
		});

	glfwGetCursorPos(m_Window.get(), &m_LastMouseX, &m_LastMouseY);
}

void Application::Cleanup()
{
	m_Engine->Cleanup();
	glfwTerminate();
}