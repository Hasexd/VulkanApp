-- Solution and Project Settings
workspace "VulkanRayTracer"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "VulkanRayTracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- GLFW Vendor
project "GLFW"
    location "vendors/glfw"
    kind "StaticLib"
    language "C"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendors/glfw/include/GLFW/**.h",
        "vendors/glfw/src/**.c"
    }

    filter "system:windows"
        systemversion "latest"

        defines { 
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"


-- vk-bootstrap Vendor
project "vk-bootstrap"
    location "vendors/vk-bootstrap"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendors/vk-bootstrap/include/**.cpp",
        "vendors/vk-bootstrap/include/**.h"
    }
    includedirs{
        "vendors/vk-bootstrap/include",
        "$(VULKAN_SDK)/Include"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

project "ImGui"
    location "vendors/imgui"
    kind "StaticLib"
    language "C++"
    staticruntime "on"
    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendors/imgui/**.cpp",
        "vendors/imgui/**.h"
    }

    includedirs{
        "vendors/glfw/include",
        "$(VULKAN_SDK)/Include"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"


-- Main Project
project "VulkanRayTracer"
    location "src"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.h",
        "src/**.cpp",
        "src/Vulkan/**.cpp",
        "src/Vulkan/**.h"
    }

    includedirs {
        "vendors/glfw/include",
        "vendors/glm",
        "vendors/vk-bootstrap/include",
        "vendors/VulkanMemoryAllocator/include",
        "vendors/imgui",
        "$(VULKAN_SDK)/Include",
        "src/Vulkan"

    }

    links {
        "GLFW",
        "ImGui",
        "vk-bootstrap"
    }

     vpaths {
    { ["_Vulkan"]       = { "src/Vulkan/**" } },
    { ["Header Files"] = { "src/**.h" } },
    { ["Source Files"] = { "src/**.cpp" } },
}

    filter "system:windows"
        systemversion "latest"
        links { "$(VULKAN_SDK)/Lib/vulkan-1" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
