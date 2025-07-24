#pragma once


#include <print>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <gtc/type_ptr.hpp>

#include <json.hpp>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "VulkanEngine.h"
#include "Renderer.h"
#include "Scene.h"

class Application
{
public:
	Application(uint32_t width, uint32_t height, const char* title, bool resizable = true, bool maximized = false, const std::string& defaultScene = "");
	~Application();

	void Run();
private:
	void Init(uint32_t width, uint32_t height, const char* title, bool resizable, bool maximized);

	void HandleCameraRotate(Camera& camera);
	void HandleKeyboardInput(Camera& camera);
	void HandleCursorInput();

	void LoadJSONScenes();
	void SaveJSONScenes();

	void DrawImGui();
private:
	uint32_t m_Width, m_Height;

	bool m_IsRunning = true;

	std::shared_ptr<GLFWwindow> m_Window;
	std::unique_ptr<Renderer> m_Renderer;

	double m_DeltaTime;

	double m_LastMouseX = 0.f;
	double m_LastMouseY = 0.f;
	bool m_FirstMouse = true;
	bool m_CursorVisible = false;
	bool m_EscapePressed = false;
	bool m_LeftClickPressed = false;
	bool m_QPressed = false;
	bool m_EPressed = false;
	bool m_ViewportHovered = false;

	uint32_t m_ViewportWidth;
	uint32_t m_ViewportHeight;

	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::unordered_map<std::string, std::string> m_SceneFilePaths;

	std::shared_ptr<Scene> m_CurrentScene;
	std::string m_CurrentSceneName;
};