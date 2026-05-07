# PathTracer

A path tracer build with Vulkan, written in C++23.

# Overview

A GPU accelerated path tracer using the Vulkan API. Scenes are rendered via compute shaders compiled to SPIR-V at build time. This project also features the ability to hot-reload shaders.

# Requirements
- CMake 3.20+
- C++23-capable compiler
- Vulkan SDK with glslansValidator on PATH
- GPU with Vulkan support

# Build

```bash
git clone --recursive https://github.com/Hasexd/PathTracer
cd PathTracer
cmake -B build
cmake --build build
```

The compiled binary is placed in build/bin/

# Controls

The user can be in one of two modes:

- **Captured** (cursor hidden): WASD to move, mouse to look around. Any movement resets image accumulation. Press ESC to switch to UI mode.
- **UI** (cursor visible): Interact with the GUI. Click anywhere outside the GUI to switch to Captured mode.

This prevents the user from accidentally moving or looking around while waiting for the image to accumulate.

# Shader hot-reloading

Modifying any of the shader files in the `/shaders` directory while the app is running will trigger the FileWatcher, printing the appropriate message in the console. Press `CTRL + R` to hot-reload all the changed shaders at once.

# Gallery
<img width="1916" height="1007" alt="image" src="https://github.com/user-attachments/assets/a2c830e1-e1ff-4216-bf1e-7a10605adfb0" />
<img width="1917" height="1004" alt="image" src="https://github.com/user-attachments/assets/14b65b47-a12a-49d5-ba63-6e2b8960c46a" />


