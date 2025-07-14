-- Solution and Project Settings
workspace "VulkanRayTracer"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "VulkanRayTracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "GLFW"
    location "vendor/glfw"
    kind "StaticLib"
    language "C"
    staticruntime "on"

    targetdir ("bin/target/" .. outputdir .. "/%{prj.name}")
    objdir ("bin/obj/" .. outputdir .. "/%{prj.name}")

    files {
        "vendor/glfw/include/GLFW/**.h",
        "vendor/glfw/src/**.c"
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
        "$(VULKAN_SDK)/Include"
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
        "$(VULKAN_SDK)/Include"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"


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
        "src/Vulkan/**.h",
        "shaders/**.comp",
        "shaders/**.vert",
        "shaders/**.frag",
        "scenes/**.json"
    }

    includedirs {
        "vendor/glfw/include",
        "vendor/glm",
        "vendor/vk-bootstrap/include",
        "vendor/VulkanMemoryAllocator/include",
        "vendor/imgui",
        "$(VULKAN_SDK)/Include",
        "src/Vulkan",
        "vendor/json/single_include"

    }

    links {
        "GLFW",
        "ImGui",
        "vk-bootstrap"
    }

     vpaths {
     {["_Vulkan"]  = { "src/Vulkan/**" }},
     {["_Shaders"] = { "shaders/**.comp", "shaders/**.vert", "shaders/**.frag" }},
     {["_Scenes"] = { "scenes/**.json" }},
     {["Header Files"] = { "src/**.h" }} ,
     {["Source Files"] = { "src/**.cpp" }},
    }

    
    filter "files:shaders/*.comp"
        buildmessage 'Compiling shader: %{file.basename}'
        buildcommands {
            'glslang -V "%{file.abspath}" -o "%{wks.location}/shaders/compiled/%{file.basename}.spv"'
        }
        buildoutputs {
            "%{wks.location}/shaders/compiled/%{file.basename}.spv"
        }
    filter {}

    filter { "system:windows" }
        systemversion "latest"
        links { "$(VULKAN_SDK)/Lib/vulkan-1" }
    filter {}

    filter {"system:linux"}
        links { "vulkan" }
    filter {}
       
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
    filter {}

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
    filter {}