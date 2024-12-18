#include "Renderer.h"


Renderer::Renderer(GLFWwindow* window):
	m_RenderData(std::make_unique<RenderData>()), m_Window(window)
{
	InitVulkan();
}


void Renderer::InitVulkan()
{
	InitDevice();
}

void Renderer::InitDevice()
{
	if(!m_Window)
	{
		printf("A window hasn't been initialized");
		return;
	}

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder.use_default_debug_messenger().request_validation_layers().build();

	if(!instanceRet)
	{
		printf("%s\n", instanceRet.error().message().c_str());
		return;
	}

	m_Instance = instanceRet.value();
	m_InstanceDispatchTable = m_Instance.make_table();
	
	CreateSurfaceGLFW(m_Instance, m_Window);

	vkb::PhysicalDeviceSelector physDeviceSelector(m_Instance);
	auto physDeviceRet = physDeviceSelector.set_surface(m_Surface).select();

	if(!physDeviceRet)
	{
		printf("%s\n", physDeviceRet.error().message().c_str());
		return;
	}

	vkb::PhysicalDevice physicalDevice = physDeviceRet.value();

	vkb::DeviceBuilder deviceBuilder(physicalDevice);

	auto deviceRet = deviceBuilder.build();
	if(!deviceRet)
	{
		printf("%s\n", deviceRet.error().message().c_str());
		return;
	}

	m_Device = deviceRet.value();
	m_DispatchTable = m_Device.make_table();
}

void Renderer::CreateSurfaceGLFW(VkInstance instance, GLFWwindow* window, VkAllocationCallbacks* allocator /* = nullptr */) 
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);

	if(err)
	{
		const char* errorMessage;
		int ret = glfwGetError(&errorMessage);
		if(ret != 0)
		{
			printf("%d ", ret);

			if(errorMessage != nullptr)
			{
				printf("%s\n", errorMessage);
			}
		}
		surface = VK_NULL_HANDLE;
	}

	m_Surface = std::move(surface);
}