# VulkanRenderingAdvanced
Advanced rendering with Vulkan API

<br />

This project demonstrates how to use a vertex shader, a geometry shader and a fragment shader to render some graphics with Vulkan API. And it also shows the usage of timestamp query and occlusion query.

This project is built on Windows 11 with Visual Studio 2022 Community Edition and Vulkan SDK 1.3. If you want to generate the SPV files, just run glsl_builder.bat.

The official Vulkan SDK for Windows is here: https://vulkan.lunarg.com/sdk/home#windows

It is also convenient to be ported to other platforms, such as Android and other Linux operating systems. If GCC or Clang is used, just use **`-std=gnu2x`** option.

