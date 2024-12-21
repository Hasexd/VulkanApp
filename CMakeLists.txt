cmake_minimum_required(VERSION 3.20)

# Project setup
project(VulkanApp LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings for compilers
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /permissive-)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Set Vulkan SDK path (required on Windows)
if (WIN32 AND NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable not set.")
endif()

# Visual Studio-specific settings
if (MSVC)
    # Set the solution directory structure for Visual Studio
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/bin")
endif()

# Add submodules
add_subdirectory(vendors/glfw)
add_subdirectory(vendors/vk-bootstrap)
add_subdirectory(vendors/glm)
add_subdirectory(vendors/VulkanMemoryAllocator)

# Include directories
include_directories(vendors/VulkanMemoryAllocator/include vendors/glm)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add source files
file(GLOB SOURCES
    src/*.cpp
)

# Add executable
add_executable(VulkanApp ${SOURCES})

# Link libraries
target_link_libraries(VulkanApp
    Vulkan::Vulkan
    glfw
    vk-bootstrap
)

# Platform-specific configurations
if (WIN32)
    target_compile_definitions(VulkanApp PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(VulkanApp PRIVATE VK_USE_PLATFORM_METAL_EXT)
elseif (UNIX)
    target_compile_definitions(VulkanApp PRIVATE VK_USE_PLATFORM_XCB_KHR)
endif()

# Set output directories
set_target_properties(VulkanApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Group sources for Visual Studio
source_group("Source Files" FILES ${SOURCES})

# Shader compilation
file(GLOB SHADER_SOURCES "${CMAKE_SOURCE_DIR}/shaders/*")
set(SPIRV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SPIRV_OUTPUT_DIR})

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" SPIRV_OUTPUT_NO_EXT ${SHADER_NAME})

    set(SPIRV_OUTPUT_FILE ${SPIRV_OUTPUT_DIR}/${SPIRV_OUTPUT_NO_EXT}.spv)

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT_FILE}
        COMMAND Vulkan::glslc ${SHADER} -o ${SPIRV_OUTPUT_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER}"
    )

    list(APPEND SPIRV_OUTPUT_FILES ${SPIRV_OUTPUT_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_OUTPUT_FILES})

# Make VulkanApp depend on shader compilation
add_dependencies(VulkanApp Shaders)

# Print summary
message(STATUS "VulkanApp Configuration:")
message(STATUS "  Vulkan SDK Path: $ENV{VULKAN_SDK}")
message(STATUS "  Source Files: ${SOURCES}")
message(STATUS "  Shader Files: ${SHADER_SOURCES}")