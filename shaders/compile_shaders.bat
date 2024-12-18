@echo off
REM Check if Vulkan SDK is set up
if "%VULKAN_SDK%"=="" (
    echo Vulkan SDK is not set up. Please ensure VULKAN_SDK is configured in your environment variables.
    exit /b 1
)

REM Create an output directory for compiled shaders if it doesn't exist
if not exist "shaders" (
    mkdir shaders
)

REM Compile vertex shader
echo Compiling vert.vert...
"%VULKAN_SDK%\Bin\glslc.exe" shaders/vert.vert -o shaders/vert.spv
if errorlevel 1 (
    echo Failed to compile vert.vert
    exit /b 1
)

REM Compile fragment shader
echo Compiling frag.frag...
"%VULKAN_SDK%\Bin\glslc.exe" shaders/frag.frag -o shaders/frag.spv
if errorlevel 1 (
    echo Failed to compile frag.frag
    exit /b 1
)

echo Compilation successful. SPIR-V binaries are in the 'shaders' directory.
pause