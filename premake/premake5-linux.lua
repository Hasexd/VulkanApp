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

    filter "system:linux"
        systemversion "latest"

        defines { 
            "_GLFW_X11",
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
        os.getenv("VULKAN_SDK") .. "/include"
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
        os.getenv("VULKAN_SDK") .. "/include"
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
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "vendors/glfw/include",
        "vendors/glm",
        "vendors/vk-bootstrap/include",
        "vendors/VulkanMemoryAllocator/include",
        "vendors/imgui",
        os.getenv("VULKAN_SDK") .. "/include"

    }

    links {
        "GLFW",
        "ImGui",
        "vk-bootstrap",
        "dl",
        "pthread"
    }

    filter "system:linux"
        systemversion "latest"
        links { os.getenv("VULKAN_SDK") .. "/lib/libvulkan.so" } -- Link Vulkan SDK library

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"