cmake_minimum_required(VERSION 3.20)

project(VulkanRayTracer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)

add_library(vk-bootstrap STATIC)
file(GLOB VK_BOOTSTRAP_SOURCES 
    vendor/vk-bootstrap/include/*.cpp
    vendor/vk-bootstrap/include/*.h
)
target_sources(vk-bootstrap PRIVATE ${VK_BOOTSTRAP_SOURCES})
target_include_directories(vk-bootstrap PUBLIC 
    vendor/vk-bootstrap/include
    ${Vulkan_INCLUDE_DIRS}
)

add_library(ImGui STATIC)
file(GLOB IMGUI_SOURCES 
    vendor/imgui/*.cpp
    vendor/imgui/*.h
)
target_sources(ImGui PRIVATE ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC 
    vendor/imgui
    vendor/glfw/include
    ${Vulkan_INCLUDE_DIRS}
)

add_executable(VulkanRayTracer)

file(GLOB_RECURSE VULKAN_RAYTRACER_SOURCES 
    src/*.cpp
    src/*.h
    src/Vulkan/*.cpp
    src/Vulkan/*.h
)

target_sources(VulkanRayTracer PRIVATE ${VULKAN_RAYTRACER_SOURCES})

target_include_directories(VulkanRayTracer PRIVATE
    vendor/glfw/include
    vendor/glm
    vendor/vk-bootstrap/include
    vendor/VulkanMemoryAllocator/include
    vendor/imgui
    ${Vulkan_INCLUDE_DIRS}
    src/Vulkan
    vendor/json/single_include
)

target_link_libraries(VulkanRayTracer PRIVATE
    glfw
    ImGui
    vk-bootstrap
    ${Vulkan_LIBRARIES}
)

target_compile_definitions(VulkanRayTracer PRIVATE 
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_options(VulkanRayTracer PRIVATE
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3>
)

if(MSVC)
    target_compile_options(VulkanRayTracer PRIVATE
            $<$<CONFIG:Debug>:/Zi /Od>
            $<$<CONFIG:Release>:/O2>
    )
    target_compile_options(VulkanRayTracer PRIVATE /MP)
endif()

set_target_properties(VulkanRayTracer PROPERTIES
        DEBUG_POSTFIX "_d"
)

find_program(GLSLANG glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")

if(NOT GLSLANG)
    message(FATAL_ERROR "Could not find glslangValidator. Make sure Vulkan SDK is installed.")
endif()

message(STATUS "Using glslangValidator: ${GLSLANG}")

file(GLOB SHADER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
)

message(STATUS "Found these shaders:")
foreach(SHADER ${SHADER_SOURCES})
    message(STATUS "  - ${SHADER}")
endforeach()

set(SHADER_OUTPUTS "")

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compiled/${SHADER_NAME}.spv)
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compiled
        COMMAND ${GLSLANG} -V ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
        VERBATIM
    )
    
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(VulkanRayTracer Shaders)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VulkanRayTracer)