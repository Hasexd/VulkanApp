#include "Application.h"

#include "imgui_internal.h"


namespace
{
	void ErrorCallback(int error, const char* description)
	{
		std::println("Error: {}\n", description);
	}
}


Application::Application(uint32_t width, uint32_t height, const char* title, bool resizable, bool maximized, const std::string& defaultScene) :
	m_Window(nullptr, glfwDestroyWindow)
{
	Init(width, height, title, resizable, maximized);
	LoadJSONScenes();

	if (!defaultScene.empty())
	{
		const auto& it = m_Scenes.find(defaultScene);

		if (it != m_Scenes.end() && it->second != nullptr)
		{
			m_CurrentScene = it->second;
			m_Renderer->SetScene(it->second);
		}
	}
}


void Application::Run()
{
	double lastFrame = glfwGetTime();

	while (m_IsRunning)
	{
		double currentFrame = glfwGetTime();
		m_DeltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		HandleCursorInput();
		
		
		bool sceneChanged = false;
		if (!m_Scenes.empty())
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetColorU32(ImVec4(0.10f, 0.10f, 0.12f, 1.0f)));

			ImGui::Begin("Scene selection");

			ImGui::BeginChild("##ScenesList", ImVec2(0, 200), true, ImGuiWindowFlags_NoNavFocus);

			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.25f, 0.30f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.30f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.35f, 0.40f, 1.0f));

			for (const auto& [name, scenePtr] : m_Scenes)
			{
				bool isCurrent = (name == m_CurrentSceneName);
				if (ImGui::Selectable(name.c_str(), isCurrent, ImGuiSelectableFlags_SpanAllColumns))
				{
					m_CurrentSceneName = name;
					m_CurrentScene = scenePtr;
					m_Renderer->SetScene(scenePtr);
					m_Renderer->ResetAccumulation();
				}
			}

			ImGui::PopStyleColor(3);
			ImGui::EndChild();
			ImGui::End();

			ImGui::PopStyleColor();
			ImGui::PopStyleVar(2);
		}

		
		if (m_CurrentScene)
		{

			Camera& camera = m_CurrentScene->GetActiveCamera();

			HandleCameraRotate(camera);
			HandleKeyboardInput(camera);

			ImGui::Begin("Scene Components");

			for (size_t i = 0; i < m_CurrentScene->GetSpheres().size(); i++)
			{
				Sphere& sphere = m_CurrentScene->GetSpheres()[i];
				Material& material = m_CurrentScene->GetMaterials()[sphere.GetMaterialIndex()];

				ImGui::PushID(static_cast<int>(i));

				if (ImGui::DragFloat3("Position", glm::value_ptr(sphere.GetPosition()), 0.1f))
					sceneChanged = true;
				if (ImGui::DragFloat("Radius", &sphere.GetRadius(), 0.1f))
					sceneChanged = true;
				if (ImGui::ColorEdit3("Color", glm::value_ptr(material.Color)))
					sceneChanged = true;
				if (ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f))
					sceneChanged = true;

				if (material.IsEmissive())
				{
					if (ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor)))
						sceneChanged = true;
					if (ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f, std::numeric_limits<float>::max()))
						sceneChanged = true;
				}

				ImGui::Separator();
				ImGui::PopID();
			}
			ImGui::End();


			ImGui::Begin("Information");
			ImGui::Text("Rendering took: %.3fms", m_LastFrameRenderTime);

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

			ImGui::InputScalar("Amount of samples", ImGuiDataType_U32, &m_Renderer->GetMaxSamples());
			ImGui::InputScalar("Maximum ray bounces", ImGuiDataType_U32, &m_Renderer->GetMaxRayBounces());

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
		}

		ImGui::Render();

		if (sceneChanged && m_Renderer->IsAccumulationEnabled())
		{
			m_Renderer->ResetAccumulation();
		}

		Render();
	}
}

void Application::Render()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	m_Renderer->Render();
	auto endTime = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float, std::milli> duration = endTime - startTime;
	m_LastFrameRenderTime = duration.count();
}

void Application::HandleKeyboardInput(Camera& camera)
{
	if (!m_CurrentScene)
		return;

	if (glfwGetKey(m_Window.get(), GLFW_KEY_Q) == GLFW_PRESS && !m_QPressed)
	{
		m_CurrentScene->SwitchCamera(-1);
		m_QPressed = true;
		if (m_Renderer->IsAccumulationEnabled())
			m_Renderer->ResetAccumulation();
	}
	else if (glfwGetKey(m_Window.get(), GLFW_KEY_Q) == GLFW_RELEASE)
	{
		m_QPressed = false;
	}

	if (glfwGetKey(m_Window.get(), GLFW_KEY_E) == GLFW_PRESS && !m_EPressed)
	{
		m_CurrentScene->SwitchCamera(1);
		m_EPressed = true;
		if (m_Renderer->IsAccumulationEnabled())
			m_Renderer->ResetAccumulation();
	}
	else if (glfwGetKey(m_Window.get(), GLFW_KEY_E) == GLFW_RELEASE)
	{
		m_EPressed = false;
	}

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
		camera.Move(movement, m_DeltaTime);

		if (m_Renderer->IsAccumulationEnabled())
			m_Renderer->ResetAccumulation();
	}
}


void Application::HandleCameraRotate(Camera& camera)
{
	if (m_CursorVisible || !m_CurrentScene)
		return;

	double xpos, ypos;
	glfwGetCursorPos(m_Window.get(), &xpos, &ypos);

	if (m_FirstMouse) {
		m_LastMouseX = xpos;
		m_LastMouseY = ypos;
		m_FirstMouse = false;
		return;
	}

	if (m_LastMouseX != xpos && m_LastMouseY != ypos)
	{
		double xOffset = m_LastMouseX - xpos;
		double yOffset = m_LastMouseY - ypos;
		m_LastMouseX = xpos;
		m_LastMouseY = ypos;

		camera.Rotate(xOffset, yOffset);

		if (m_Renderer->IsAccumulationEnabled())
			m_Renderer->ResetAccumulation();
	}

	
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

void Application::Init(uint32_t width, uint32_t height, const char* title, bool resizable, bool maximized)
{
	if (!glfwInit())
	{
		std::println("Couldn't initialize GLFW");
		return;
	}
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, resizable);
	glfwWindowHint(GLFW_MAXIMIZED, maximized);

	m_Window = std::shared_ptr<GLFWwindow>(glfwCreateWindow(width, height, title, nullptr, nullptr), glfwDestroyWindow);
	m_Width = width;
	m_Height = height;

	m_Renderer = std::make_unique<Renderer>(m_Window, m_Width, m_Height);

	glfwSetWindowUserPointer(m_Window.get(), this);
	glfwSetInputMode(m_Window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetFramebufferSizeCallback(m_Window.get(), [](GLFWwindow* window, int width, int height) -> void
		{
			if (const auto app = static_cast<Application*>(glfwGetWindowUserPointer(window)))
			{
				app->m_Width = width;
				app->m_Height = height;
				app->m_Renderer->Resize(width, height);
			}
		});

	glfwSetWindowCloseCallback(m_Window.get(), [](GLFWwindow* window) -> void
		{
			if (const auto app = static_cast<Application*>(glfwGetWindowUserPointer(window)))
			{
				app->m_IsRunning = false;
			}
		});

	glfwGetCursorPos(m_Window.get(), &m_LastMouseX, &m_LastMouseY);
}

void Application::LoadJSONScenes()
{
	using json = nlohmann::json;
	std::filesystem::path path = "../scenes";

	for (const auto& file : std::filesystem::directory_iterator(path))
	{
		if (file.path().extension().string() == ".json")
		{
			std::ifstream stream(file.path());
			std::string fileName = file.path().filename().string();

			std::string sceneName = fileName.substr(0, fileName.find('.'));

			if (stream.is_open())
			{
				json fileContents = json::parse(stream);
				Scene scene;
				const auto& camerasJson = fileContents["Cameras"];


				if (camerasJson.type() == json::value_t::array)
				{
					scene.GetCameras().reserve(camerasJson.size());
					for (const auto& cameraJson : camerasJson)
					{
						glm::vec3 position;
						float yaw, pitch;
						float fov;

						if (cameraJson.contains("Position"))
						{
							glm::vec3 cameraPosition = {
								cameraJson["Position"][0],
								cameraJson["Position"][1],
								cameraJson["Position"][2]
							};

							position = cameraPosition;
						}

						if (cameraJson.contains("Rotation"))
						{
							yaw = cameraJson["Rotation"][0];
							pitch = cameraJson["Rotation"][1];
						}

						if (cameraJson.contains("FieldOfView"))
						{
							fov = cameraJson["FieldOfView"];
						}
						scene.GetCameras().emplace_back(position, yaw, pitch, fov);
					}
				}

				if (fileContents.contains("ActiveCameraIndex"))
				{
					scene.SetActiveCameraIndex(fileContents["ActiveCameraIndex"]);
				}
				else
				{
					scene.SetActiveCameraIndex(0);
				}

				const auto& spheresJson = fileContents["Spheres"];
				const auto& materialsJson = fileContents["Materials"];

				if (materialsJson.type() == json::value_t::array)
				{
					scene.GetMaterials().reserve(materialsJson.size());

					for (const auto& jsonMaterial : materialsJson)
					{
						const glm::vec3 color = { jsonMaterial["Color"][0], jsonMaterial["Color"][1],
							jsonMaterial["Color"][2]};

						const float roughness = jsonMaterial["Roughness"];
						const float metallic = jsonMaterial["Metallic"];
						const float emissionPower = jsonMaterial["EmissionPower"];

						if (emissionPower > 0.0f)
						{
							const glm::vec3 emissionColor = { jsonMaterial["EmissionColor"][0], jsonMaterial["EmissionColor"][1],
								jsonMaterial["EmissionColor"][2] };
							scene.GetMaterials().emplace_back(color, roughness, metallic, emissionColor, emissionPower);
						}
						else
						{
							scene.GetMaterials().emplace_back(color, roughness, metallic);
						}
					}
				}

				if (spheresJson.type() == json::value_t::array)
				{
					scene.GetSpheres().reserve(spheresJson.size());
					for (const auto& jsonSphere : spheresJson)
					{
						const glm::vec3 position = { jsonSphere["Position"][0], jsonSphere["Position"][1], jsonSphere["Position"][2]};
						const float radius = jsonSphere["Radius"];
						const uint32_t materialIndex = jsonSphere["MaterialIndex"];

						scene.GetSpheres().emplace_back(position, radius, materialIndex);
					}
				}

				m_Scenes[sceneName] = std::make_shared<Scene>(scene);
				m_SceneFilePaths[sceneName] = file.path().string();

				stream.close();
			}
		}
	}
}

void Application::SaveJSONScenes()
{
	using json = nlohmann::json;

	for (const auto& [sceneName, scenePtr] : m_Scenes)
	{
		auto pathIt = m_SceneFilePaths.find(sceneName);
		if (pathIt == m_SceneFilePaths.end())
		{
			std::println("Warning: Could not find file path for scene '{}'", sceneName);
			continue;
		}

		const std::string& filePath = pathIt->second;

		json sceneJson;

		json materialsJson = json::array();
		for (const auto& material : scenePtr->GetMaterials())
		{
			json materialJson;
			materialJson["Color"] = { material.Color.r, material.Color.g, material.Color.b };
			materialJson["Roughness"] = material.Roughness;
			materialJson["Metallic"] = material.Metallic;
			materialJson["EmissionPower"] = material.EmissionPower;

			if (material.IsEmissive())
			{
				materialJson["EmissionColor"] = { material.EmissionColor.r, material.EmissionColor.g, material.EmissionColor.b };
			}
			else
			{
				materialJson["EmissionColor"] = { 0.0f, 0.0f, 0.0f };
			}

			materialsJson.push_back(materialJson);
		}
		sceneJson["Materials"] = materialsJson;

		json spheresJson = json::array();
		for (const auto& sphere : scenePtr->GetSpheres())
		{
			json sphereJson;
			glm::vec3 pos = sphere.GetPosition();
			sphereJson["Position"] = { pos.x, pos.y, pos.z };
			sphereJson["Radius"] = sphere.GetRadius();
			sphereJson["MaterialIndex"] = sphere.GetMaterialIndex();

			spheresJson.push_back(sphereJson);
		}
		sceneJson["Spheres"] = spheresJson;

		for (const Camera& camera: scenePtr->GetCameras())
		{
			json cameraJson;
			cameraJson["Position"] = { camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z };
			cameraJson["Rotation"] = { camera.GetPitch(), camera.GetYaw() };
			cameraJson["FieldOfView"] = camera.GetFieldOfView();
			sceneJson["Cameras"].push_back(cameraJson);
		}

		sceneJson["ActiveCameraIndex"] = scenePtr->GetActiveCameraIndex();

		std::ofstream outFile(filePath);
		if (outFile.is_open())
		{
			outFile << sceneJson.dump(4);
			outFile.close();
		}
	}
}

Application::~Application()
{
	SaveJSONScenes();
	glfwTerminate();
}

