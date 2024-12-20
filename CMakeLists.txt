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

# Add submodules
add_subdirectory(vendors/glfw)
add_subdirectory(vendors/vk-bootstrap)

# Include VulkanMemoryAllocator header-only library
include_directories(vendors/VulkanMemoryAllocator/include)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/VulkanEngine.cpp
)
# Add executable
add_executable(VulkanApp ${SOURCES})

# Link libraries
# Vulkan, GLFW, vk-bootstrap, GLM (header-only, no linking required)
target_link_libraries(VulkanApp
    Vulkan::Vulkan
    glfw
    vk-bootstrap
)

# Include directories
# GLFW and GLM are header-only; VulkanMemoryAllocator is included above
target_include_directories(VulkanApp PRIVATE
    vendors/glm
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

# Print summary
message(STATUS "VulkanApp Configuration:")
message(STATUS "  Vulkan SDK Path: $ENV{VULKAN_SDK}")
message(STATUS "  Source Files: ${SOURCES}")

