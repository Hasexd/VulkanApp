-- Solution and Project Settings
workspace "VulkanRayTracer"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "VulkanRayTracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vulkan = os.getenv("VULKAN_SDK")

-- GLFW Vendor
project "GLFW"
    location "vendor/glfw"
    kind "StaticLib"
    language "C"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    filter "system:linux"
    	pic "On"
        systemversion "latest"

    files {
            "vendor/glfw/src/**.c"
        }

        defines { 
            "_GLFW_X11",
            "_CRT_SECURE_NO_WARNINGS"
        }
        
        links {
            "dl",
            "pthread",
            "X11",
            "Xrandr",
            "Xinerama",
            "Xcursor",
            "Xext"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"


-- vk-bootstrap Vendor
project "vk-bootstrap"
    location "vendor/vk-bootstrap"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendor/vk-bootstrap/include/**.cpp",
        "vendor/vk-bootstrap/include/**.h"
    }
    includedirs{
        "vendor/vk-bootstrap/include",
        vulkan .. "/include"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

project "ImGui"
    location "vendor/imgui"
    kind "StaticLib"
    language "C++"
    staticruntime "on"
    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendor/imgui/**.cpp",
        "vendor/imgui/**.h"
    }

    includedirs{
        "vendor/glfw/include",
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
        "vendor/glfw/include",
        "vendor/glm",
        "vendor/vk-bootstrap/include",
        "vendor/VulkanMemoryAllocator/include",
        "vendor/imgui",
        vulkan .. "/include",
        "src/Vulkan",
        "vendor/json/single_include"

    }

    links {
        "GLFW",
        "ImGui",
        "vk-bootstrap"
    }

    filter "system:linux"
        systemversion "latest"
        links { "vulkan" } -- Link Vulkan SDK library

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
