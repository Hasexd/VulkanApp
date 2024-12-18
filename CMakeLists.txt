cmake_minimum_required(VERSION 3.10)
project(VulkanApp LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Project structure
set(SOURCE_FILES
    "src/main.cpp"
 "src/Window.cpp" "src/App.cpp" "src/Renderer.cpp")

# Define the vendors directory path
set(VENDORS_DIR ${CMAKE_SOURCE_DIR}/vendors)



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
    ${VULKAN_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/headers
)


# Project executable
add_executable(VulkanApp ${SOURCE_FILES})

add_subdirectory(vendors/vk-bootstrap)
add_subdirectory(vendors/glfw)

# Link libraries
target_link_libraries(VulkanApp
    ${VULKAN_LIBRARY}
    vk-bootstrap::vk-bootstrap
    glfw
)

target_include_directories(VulkanApp PUBLIC vendors/glm)


# OS-specific settings
if(WIN32)
    target_compile_definitions(VulkanApp PRIVATE GLEW_STATIC)
elseif(UNIX)
    target_link_libraries(VulkanApp dl pthread) # Required for Linux
endif()
