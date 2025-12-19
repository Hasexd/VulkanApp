#include "Renderer.h"


Renderer::Renderer(const std::shared_ptr<GLFWwindow>& window, uint32_t width, uint32_t height) :
	m_Width(width), m_Height(height), m_AspectRatio((float)m_Width / m_Height),
	m_MaxSamples(250), m_MaxRayBounces(10)
{

	m_Engine = std::make_unique<VulkanEngine>();
	m_Engine->Init(window);
}

Renderer::~Renderer()
{
	if (m_Engine)
	{
		m_Engine->Cleanup();
	}
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height) const
{
	m_Engine->OnWindowResize(width, height);
}

void Renderer::ResizeViewport(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
		return;

	m_Width = width;
	m_Height = height;

	m_AspectRatio = static_cast<float>(m_Width) / m_Height;
	m_SampleCount = 1;

	m_Engine->SetViewportSize(width, height);
}

void Renderer::Render()
{
	if(const auto& scene = m_CurrentScene.lock(); m_DispatchCompute = scene && !IsComplete())
	{
		UpdateUniformBuffer(scene);
		UpdateSphereBuffer(scene);
		UpdatePlaneBuffer(scene);
		UpdateMaterialBuffer(scene);

		if (m_AccumulationEnabled)
			++m_SampleCount;
	}

	m_Engine->DrawFrame(glm::vec2(0.0f), m_DispatchCompute);
}

void Renderer::UpdateUniformBuffer(const std::shared_ptr<Scene>& scene) const
{
	if (!scene) 
		return;

	const Camera& camera = scene->GetActiveCamera();
	UniformBufferData ubo = {};

	ubo.CameraPosition = camera.GetPosition();
	ubo.CameraFrontVector = camera.GetDirection();
	ubo.CameraUpVector = camera.GetUp();
	ubo.CameraRightVector = camera.GetRight();
	ubo.AspectRatio = m_AspectRatio;
	ubo.FieldOfView = camera.GetFieldOfView();
	ubo.SampleCount = m_SampleCount;
	ubo.MaxBounces = m_MaxRayBounces;
	ubo.BackgroundColor = m_BackgroundColor;
	ubo.Width = m_Width;
	ubo.Height = m_Height;
	ubo.AccumulationEnabled = m_AccumulationEnabled;

	void* data;
	vmaMapMemory(m_Engine->GetAllocator(), m_Engine->UniformBuffer.Allocation, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vmaUnmapMemory(m_Engine->GetAllocator(), m_Engine->UniformBuffer.Allocation);
}

void Renderer::UpdateSphereBuffer(const std::shared_ptr<Scene>& scene) const
{
	if (!scene) 
		return;

	const auto& spheres = scene->GetSpheres();
	std::vector<SphereBufferData> gpuSpheres;
	gpuSpheres.reserve(spheres.size());

	for (const auto& sphere : spheres) 
	{
		SphereBufferData sd = {};
		sd.Position = sphere.GetPosition();
		sd.Radius = sphere.GetRadius();
		sd.MaterialIndex = sphere.GetMaterialIndex();
		gpuSpheres.push_back(sd);
	}

	void* data;
	vmaMapMemory(m_Engine->GetAllocator(), m_Engine->SphereBuffer.Allocation, &data);
	memcpy(data, gpuSpheres.data(), gpuSpheres.size() * sizeof(SphereBufferData));
	vmaUnmapMemory(m_Engine->GetAllocator(), m_Engine->SphereBuffer.Allocation);
}

void Renderer::UpdatePlaneBuffer(const std::shared_ptr<Scene>& scene) const
{
	if (!scene)
		return;

	const auto& planes = scene->GetPlanes();
	std::vector<PlaneBufferData> gpuPlanes;
	gpuPlanes.reserve(planes.size());

	for (const auto& plane : planes)
	{
		PlaneBufferData pd = {};

		pd.Position = plane.GetPosition();
		pd.Normal = plane.GetNormal();
		pd.Width = plane.GetWidth();
		pd.Height = plane.GetHeight();
		pd.MaterialIndex = plane.GetMaterialIndex();
		gpuPlanes.push_back(pd);
	}

	void* data;
	vmaMapMemory(m_Engine->GetAllocator(), m_Engine->PlaneBuffer.Allocation, &data);
	memcpy(data, gpuPlanes.data(), gpuPlanes.size() * sizeof(PlaneBufferData));
	vmaUnmapMemory(m_Engine->GetAllocator(), m_Engine->PlaneBuffer.Allocation);
}

void Renderer::UpdateMaterialBuffer(const std::shared_ptr<Scene>& scene) const
{
	if (!scene)
		return;

	const auto& materials = scene->GetMaterials();
	std::vector<MaterialBufferData> gpuMaterials;
	gpuMaterials.reserve(materials.size());

	for (const auto& material : materials) 
	{
		MaterialBufferData md = {};
		md.Color = material.Color;
		md.Roughness = material.Roughness;
		md.Metallic = material.Metallic;
		md.Specular = material.Specular;
		md.EmissionPower = material.EmissionPower;
		gpuMaterials.push_back(md);
	}

	void* data;
	vmaMapMemory(m_Engine->GetAllocator(), m_Engine->MaterialBuffer.Allocation, &data);
	memcpy(data, gpuMaterials.data(), gpuMaterials.size() * sizeof(MaterialBufferData));
	vmaUnmapMemory(m_Engine->GetAllocator(), m_Engine->MaterialBuffer.Allocation);
}

void Renderer::ReloadShaders()
{
	m_Engine->ReloadShaders();
	ResetAccumulation();
}

void Renderer::ResetAccumulation()
{
	m_SampleCount = 1;
	m_Engine->ResetAccumulation();
}
