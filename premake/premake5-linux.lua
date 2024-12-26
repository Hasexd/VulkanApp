-- Solution and Project Settings
workspace "VulkanRayTracer"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "VulkanRayTracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vulkan = os.getenv("VULKAN_SDK")

print(vulkan .. "/include")

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
        vulkan .. "/include"
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
        vulkan .. "/include"
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
        vulkan .. "/include"

    }

    links {
        "GLFW",
        "ImGui",
        "vk-bootstrap"
    }

     vpaths {
         ["Header Files"] = {"src/**.h"},
         ["Source Files"] = {"src/**.cpp"}
     }

    filter "system:windows"
        systemversion "latest"
        links { vulkan .. "/lib/vulkan-1" } -- Link Vulkan SDK library

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
