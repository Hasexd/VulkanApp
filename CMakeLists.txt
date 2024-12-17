cmake_minimum_required(VERSION 3.10)
project(VulkanApp LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project structure
set(SOURCE_FILES
    src/main.cpp
)

# Define the vendors directory path
set(VENDORS_DIR ${CMAKE_SOURCE_DIR}/vendors)

# GLFW Setup
add_subdirectory(${VENDORS_DIR}/glfw glfw_build)

# GLEW Setup
add_library(glew STATIC
    ${VENDORS_DIR}/glew/src/glew.c
)
target_include_directories(glew PUBLIC ${VENDORS_DIR}/glew/include)

# GLM Setup (Header-only library)
set(GLM_INCLUDE_DIR ${VENDORS_DIR}/glm)
include_directories(${GLM_INCLUDE_DIR})

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

# Include Vulkan headers
include_directories(${VULKAN_INCLUDE_DIR})

# Project executable
add_executable(MyProject ${SOURCE_FILES})

# Link libraries
target_link_libraries(MyProject
    glfw
    glew
    ${VULKAN_LIBRARY}
)

# Include directories for GLFW and GLEW
target_include_directories(MyProject PRIVATE
    ${VENDORS_DIR}/glfw/include
    ${VENDORS_DIR}/glew/include
    ${GLM_INCLUDE_DIR}
)

# OS-specific settings
if(WIN32)
    target_compile_definitions(MyProject PRIVATE GLEW_STATIC)
elseif(UNIX)
    target_link_libraries(MyProject dl pthread) # Required for Linux
endif()