cmake_minimum_required(VERSION 3.10)
project(VulkanApp LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/headers)

# Project structure
set(SOURCE_FILES
    "src/main.cpp"
 "src/Window.cpp")

# Define the vendors directory path
set(VENDORS_DIR ${CMAKE_SOURCE_DIR}/vendors)

# GLFW Setup (Pre-compiled binary)
set(GLFW_INCLUDE_DIR ${VENDORS_DIR}/glfw/include)
set(GLFW_LIB ${VENDORS_DIR}/glfw/lib-vc2022/glfw3.lib) # Path to GLFW pre-compiled library

# GLEW Setup (Pre-compiled binary)
set(GLEW_INCLUDE_DIR ${VENDORS_DIR}/glew/include)
set(GLEW_LIB ${VENDORS_DIR}/glew/lib/Release/x64/glew32s.lib) # Adjust path to match your GLEW static lib

# GLM Setup (Header-only library)
set(GLM_INCLUDE_DIR ${VENDORS_DIR}/glm)

# Vulkan Setup
if(WIN32)
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    if(NOT VULKAN_SDK_PATH)
        message(FATAL_ERROR "VULKAN_SDK environment variable is not set!")
    endif()
    set(VULKAN_INCLUDE_DIR ${VULKAN_SDK_PATH}/Include)
    set(VULKAN_LIBRARY ${VULKAN_SDK_PATH}/Lib/vulkan-1.lib)
elseif(UNIX)
    find_package(Vulkan REQUIRED)
    set(VULKAN_LIBRARY Vulkan::Vulkan)
endif()

# Include directories
include_directories(
    ${GLFW_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_INCLUDE_DIR}
)

# Project executable
add_executable(VulkanApp ${SOURCE_FILES})

# Link libraries
target_link_libraries(VulkanApp
    ${GLFW_LIB}
    ${GLEW_LIB}
    ${VULKAN_LIBRARY}
)

# OS-specific settings
if(WIN32)
    target_compile_definitions(VulkanApp PRIVATE GLEW_STATIC)
elseif(UNIX)
    target_link_libraries(VulkanApp dl pthread) # Required for Linux
endif()
